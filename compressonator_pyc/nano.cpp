#include "compressonator.h"
#include <cstring>
#include <variant>
#include <optional>
#include <sstream>
#include <string>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/function.h>
#include <unordered_map>
#include <mutex>

#include "./formatinfo.hpp"

namespace nb = nanobind;

NB_MAKE_OPAQUE(std::vector<uint8_t>);
NB_MAKE_OPAQUE(std::vector<AMD_CMD_SET>);

inline void check_cmp_error(CMP_ERROR status)
{
    switch (status)
    {
    case CMP_OK:
        return;

    case CMP_ABORTED:
        throw std::runtime_error("The conversion was aborted.");

    // Input & Dimension Validation Errors -> ValueError / Exception
    case CMP_ERR_INVALID_SOURCE_TEXTURE:
        throw nb::value_error("The source texture is invalid.");
    case CMP_ERR_INVALID_DEST_TEXTURE:
        throw nb::value_error("The destination texture is invalid.");
    case CMP_ERR_UNSUPPORTED_SOURCE_FORMAT:
        throw nb::value_error("The source format is not a supported format.");
    case CMP_ERR_UNSUPPORTED_DEST_FORMAT:
        throw nb::value_error("The destination format is not a supported format.");
    case CMP_ERR_UNKNOWN_DESTINATION_FORMAT:
        throw nb::value_error("The destination Codec Type is unknown.");
    case CMP_ERR_SIZE_MISMATCH:
        throw nb::value_error("The source and destination texture sizes do not match.");
    case CMP_ERR_GAMMA_OUTOFRANGE:
        throw nb::value_error("Gamma value set for processing is out of range.");
    case CMP_ERR_NOSHADER_CODE_DEFINED:
        throw nb::value_error("No shader code is available for the requested framework.");

    // File & Library Loading Errors -> FileNotFoundError / ImportError
    case CMP_ERR_PLUGIN_FILE_NOT_FOUND:
        throw std::runtime_error("The required plugin library was not found.");
    case CMP_ERR_UNABLE_TO_LOAD_FILE:
        throw std::runtime_error("The requested file was not loaded.");
    case CMP_ERR_UNABLE_TO_LOAD_ENCODER:
        throw nb::import_error("Unable to load an encode library.");

    // Memory Allocation -> MemoryError
    case CMP_ERR_MEM_ALLOC_FOR_MIPSET:
        throw std::bad_alloc(); // Standard C++ std::bad_alloc maps automatically to Python MemoryError

    // Hardware & Subsystem Initialization -> RuntimeError
    case CMP_ERR_UNSUPPORTED_GPU_ASTC_DECODE:
        throw std::runtime_error("The GPU hardware is not supported for ASTC decode.");
    case CMP_ERR_UNSUPPORTED_GPU_BASIS_DECODE:
        throw std::runtime_error("The GPU hardware is not supported for Basis decode.");
    case CMP_ERR_GPU_DOESNOT_SUPPORT_COMPUTE:
        throw std::runtime_error("The GPU device selected does not support compute.");
    case CMP_ERR_GPU_DOESNOT_SUPPORT_CMP_EXT:
        throw std::runtime_error("The GPU does not support the requested compression extension.");
    case CMP_ERR_UNABLE_TO_INIT_CODEC:
        throw std::runtime_error("Compressonator was unable to initialize the codec needed for conversion.");
    case CMP_ERR_UNABLE_TO_INIT_DECOMPRESSLIB:
        throw std::runtime_error("GPU_Decode Lib was unable to initialize the codec needed for decompression.");
    case CMP_ERR_UNABLE_TO_INIT_COMPUTELIB:
        throw std::runtime_error("Compute Lib was unable to initialize the codec needed for compression.");
    case CMP_ERR_CMP_DESTINATION:
        throw std::runtime_error("Error in compressing destination texture.");
    case CMP_ERR_FAILED_HOST_SETUP:
        throw std::runtime_error("Failed to setup Host for processing.");
    case CMP_ERR_UNABLE_TO_CREATE_ENCODER:
        throw std::runtime_error("Request to create an encoder failed.");
    case CMP_ERR_NOPERFSTATS:
        throw std::runtime_error("No Performance Stats are available.");
    case CMP_ERR_PLUGIN_SHAREDIO_NOT_SET:
        throw std::runtime_error("The plugin C_PluginSetSharedIO call was not set and is required for this plugin to operate.");
    case CMP_ERR_UNABLE_TO_INIT_D3DX:
        throw std::runtime_error("Unable to initialize DirectX SDK or get a specific DX API.");
    case CMP_FRAMEWORK_NOT_INITIALIZED:
        throw std::runtime_error("CMP_InitFramework failed or not called.");

    // Fallback for unexpected or unknown codes
    case CMP_ERR_GENERIC:
    default:
        throw std::runtime_error("An unknown Compressonator error occurred.");
    }
}

// 1. Thread-local pointer to hold the active feedback callable per-thread safely
thread_local const std::function<bool(CMP_FLOAT)>* g_current_feedback = nullptr;

// 2. Non-capturing C-style callback function (decays natively to CMP_Feedback_Proc)
inline bool CMP_API cmp_feedback_proxy(CMP_FLOAT fProgress, CMP_DWORD_PTR pUser1, CMP_DWORD_PTR pUser2)
{
    // Re-acquire the GIL before running any Python / nanobind logic
    nb::gil_scoped_acquire acquire;

    if (g_current_feedback && *g_current_feedback)
    {
        return (*g_current_feedback)(fProgress);
    }
    return false; // Return true if you want Compressonator to abort conversion
}

struct CMP_Texture_Wrapper
{
    CMP_Texture inner;
    std::variant<std::vector<uint8_t>, nb::object> storage;

    CMP_Texture_Wrapper(
        CMP_DWORD dwWidth,
        CMP_DWORD dwHeight,
        CMP_DWORD dwPitch,
        CMP_FORMAT format,
        nb::object dwData = nb::none(),
        CMP_FORMAT transcodeFormat = CMP_FORMAT::CMP_FORMAT_MAX,
        CMP_DWORD nBlockHeight = 4,
        CMP_DWORD nBlockWidth = 4,
        CMP_DWORD nBlockDepth = 1) : inner{0}
    {
        inner.dwSize = sizeof(CMP_Texture);
        inner.dwWidth = dwWidth;
        inner.dwHeight = dwHeight;
        inner.dwPitch = dwPitch;
        inner.format = format;
        inner.transcodeFormat = transcodeFormat;
        inner.nBlockHeight = nBlockHeight;
        inner.nBlockWidth = nBlockWidth;
        inner.nBlockDepth = nBlockDepth;

        if (dwPitch != 0)
        {
            throw std::invalid_argument("dwPitch must be 0 for this wrapper; it will be calculated automatically.");
        }

        inner.dwDataSize = CMP_CalculateBufferSize(&inner);

        // 2. Handle Data Input vs. Self-Allocation
        if (dwData.is_none() == false)
        {
            try
            {
                attempt_assign<nb::ndarray<nb::array_api, nb::device::cpu>>(dwData);
            }
            catch (const std::exception &e)
            {
                attempt_assign<nb::ndarray<nb::ro, nb::array_api, nb::device::cpu>>(dwData);
            }
        }
        else
        {
            // No data provided: allocate our own internal std::vector
            if (inner.dwDataSize == 0)
            {
                throw std::invalid_argument("Buffer size is zero, cannot allocate texture data!");
            }

            storage = std::vector<uint8_t>(inner.dwDataSize);
            inner.pData = std::get<std::vector<uint8_t>>(storage).data();
        }
    }
    // Disable copy
    CMP_Texture_Wrapper(const CMP_Texture_Wrapper &) = delete;
    CMP_Texture_Wrapper &operator=(const CMP_Texture_Wrapper &) = delete;

    // Move constructor: update pData if we own a vector
    CMP_Texture_Wrapper(CMP_Texture_Wrapper &&other) noexcept
        : inner(other.inner), storage(std::move(other.storage))
    {
        if (std::holds_alternative<std::vector<uint8_t>>(storage))
        {
            inner.pData = std::get<std::vector<uint8_t>>(storage).data();
        }
        else
        {
            inner.pData = other.inner.pData;
        }
    }

    bool operator==(const CMP_Texture_Wrapper &other) const
    {
        return (inner.dwWidth == other.inner.dwWidth) &&
               (inner.dwHeight == other.inner.dwHeight) &&
               (inner.dwPitch == other.inner.dwPitch) &&
               (inner.format == other.inner.format) &&
               (inner.transcodeFormat == other.inner.transcodeFormat) &&
               (inner.nBlockHeight == other.inner.nBlockHeight) &&
               (inner.nBlockWidth == other.inner.nBlockWidth) &&
               (inner.nBlockDepth == other.inner.nBlockDepth) &&
               (inner.dwDataSize == other.inner.dwDataSize) &&
               ((inner.pData == other.inner.pData) ||
                (inner.pData && other.inner.pData && memcmp(inner.pData, other.inner.pData, inner.dwDataSize) == 0));
    }

    template <typename T>
    void attempt_assign(nb::object dwData)
    {
        // 1. Cast
        auto arr = nb::cast<T>(dwData);

        // 2. Check size using size_bytes()
        if (arr.nbytes() == 0){
            throw std::invalid_argument(
                "Input array buffer is empty!");
        }
        if (arr.nbytes() < inner.dwDataSize)
        {
            throw std::invalid_argument(
                "Input array buffer size is smaller than required texture format size!");
        }

        // 3. Assign data pointer (cast away const safely just in case T is nb::ro)
        inner.pData = const_cast<uint8_t *>(static_cast<const uint8_t *>(arr.data()));

        // 4. Hold reference
        storage = dwData;
    }

    // Move assignment operator
    CMP_Texture_Wrapper &operator=(CMP_Texture_Wrapper &&other) noexcept
    {
        if (this != &other)
        {
            inner = other.inner;
            storage = std::move(other.storage);

            if (std::holds_alternative<std::vector<uint8_t>>(storage))
            {
                inner.pData = std::get<std::vector<uint8_t>>(storage).data();
            }
            else
            {
                inner.pData = other.inner.pData;
            }
        }
        return *this;
    }

    // Get raw data pointer regardless of storage type
    uint8_t *data() { return inner.pData; }
    size_t size() const { return inner.dwDataSize; }

    auto convert(CMP_Texture_Wrapper &other, CMP_CompressOptions &options, std::function<bool(CMP_FLOAT)> feedback) -> void
    {
        CMP_ERROR result;

        if (!feedback)
        {
            std::cout << "no feedback!" << std::endl;
            // No callback provided: safe to release GIL completely
            nb::gil_scoped_release release;
            result = CMP_ConvertTexture(&inner, &other.inner, &options, nullptr);
        }
        else
        {
            // Bind the current thread's storage to the feedback function
            g_current_feedback = &feedback;

            {
                // Release GIL during heavy texture processing
                nb::gil_scoped_release release;
                result = CMP_ConvertTexture(&inner, &other.inner, &options, cmp_feedback_proxy);
            }

            // Clean up context pointer when done
            g_current_feedback = nullptr;
        }
        check_cmp_error(result);
    }
};

NB_MODULE(_compressonator, m)
{
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //      Enums
    //
    //////////////////////////////////////////////////////////////////////////////////////////

    nb::enum_<CMP_FORMAT>(m, "CMP_Format")
        .value("Unknown", CMP_FORMAT::CMP_FORMAT_Unknown, "Undefined texture format.")
        .value("RGBA_8888_S", CMP_FORMAT::CMP_FORMAT_RGBA_8888_S, "RGBA format with signed 8-bit fixed channels.")
        .value("ARGB_8888_S", CMP_FORMAT::CMP_FORMAT_ARGB_8888_S, "ARGB format with signed 8-bit fixed channels.")
        .value("ARGB_8888", CMP_FORMAT::CMP_FORMAT_ARGB_8888, "ARGB format with 8-bit fixed channels.")
        .value("ABGR_8888", CMP_FORMAT::CMP_FORMAT_ABGR_8888, "ABGR format with 8-bit fixed channels.")
        .value("RGBA_8888", CMP_FORMAT::CMP_FORMAT_RGBA_8888, "RGBA format with 8-bit fixed channels.")
        .value("BGRA_8888", CMP_FORMAT::CMP_FORMAT_BGRA_8888, "BGRA format with 8-bit fixed channels.")
        .value("RGB_888", CMP_FORMAT::CMP_FORMAT_RGB_888, "RGB format with 8-bit fixed channels.")
        .value("RGB_888_S", CMP_FORMAT::CMP_FORMAT_RGB_888_S, "RGB format with 8-bit fixed channels.")
        .value("BGR_888", CMP_FORMAT::CMP_FORMAT_BGR_888, "BGR format with 8-bit fixed channels.")
        .value("RG_8_S", CMP_FORMAT::CMP_FORMAT_RG_8_S, "Two component format with signed 8-bit fixed channels.")
        .value("RG_8", CMP_FORMAT::CMP_FORMAT_RG_8, "Two component format with 8-bit fixed channels.")
        .value("R_8_S", CMP_FORMAT::CMP_FORMAT_R_8_S, "Single component format with signed 8-bit fixed channel.")
        .value("R_8", CMP_FORMAT::CMP_FORMAT_R_8, "Single component format with 8-bit fixed channel.")
        .value("ARGB_2101010", CMP_FORMAT::CMP_FORMAT_ARGB_2101010, "ARGB format with 10-bit fixed channels for color & a 2-bit fixed channel for alpha.")
        .value("RGBA_1010102", CMP_FORMAT::CMP_FORMAT_RGBA_1010102, "RGBA format with 10-bit fixed channels for color & a 2-bit fixed channel for alpha.")
        .value("ARGB_16", CMP_FORMAT::CMP_FORMAT_ARGB_16, "ARGB format with 16-bit fixed channels.")
        .value("ABGR_16", CMP_FORMAT::CMP_FORMAT_ABGR_16, "ABGR format with 16-bit fixed channels.")
        .value("RGBA_16", CMP_FORMAT::CMP_FORMAT_RGBA_16, "RGBA format with 16-bit fixed channels.")
        .value("BGRA_16", CMP_FORMAT::CMP_FORMAT_BGRA_16, "BGRA format with 16-bit fixed channels.")
        .value("RG_16", CMP_FORMAT::CMP_FORMAT_RG_16, "Two component format with 16-bit fixed channels.")
        .value("R_16", CMP_FORMAT::CMP_FORMAT_R_16, "Single component format with 16-bit fixed channels.")

        // Float Format 0x1nn0
        .value("RGBE_32F", CMP_FORMAT::CMP_FORMAT_RGBE_32F, "RGB format with 9-bit floating point each channel and shared 5 bit exponent")
        .value("ARGB_16F", CMP_FORMAT::CMP_FORMAT_ARGB_16F, "ARGB format with 16-bit floating-point channels.")
        .value("ABGR_16F", CMP_FORMAT::CMP_FORMAT_ABGR_16F, "ABGR format with 16-bit floating-point channels.")
        .value("RGBA_16F", CMP_FORMAT::CMP_FORMAT_RGBA_16F, "RGBA format with 16-bit floating-point channels.")
        .value("BGRA_16F", CMP_FORMAT::CMP_FORMAT_BGRA_16F, "BGRA format with 16-bit floating-point channels.")
        .value("RG_16F", CMP_FORMAT::CMP_FORMAT_RG_16F, "Two component format with 16-bit floating-point channels.")
        .value("R_16F", CMP_FORMAT::CMP_FORMAT_R_16F, "Single component with 16-bit floating-point channels.")
        .value("ARGB_32F", CMP_FORMAT::CMP_FORMAT_ARGB_32F, "ARGB format with 32-bit floating-point channels.")
        .value("ABGR_32F", CMP_FORMAT::CMP_FORMAT_ABGR_32F, "ABGR format with 32-bit floating-point channels.")
        .value("RGBA_32F", CMP_FORMAT::CMP_FORMAT_RGBA_32F, "RGBA format with 32-bit floating-point channels.")
        .value("BGRA_32F", CMP_FORMAT::CMP_FORMAT_BGRA_32F, "BGRA format with 32-bit floating-point channels.")
        .value("RGB_32F", CMP_FORMAT::CMP_FORMAT_RGB_32F, "RGB format with 32-bit floating-point channels.")
        .value("BGR_32F", CMP_FORMAT::CMP_FORMAT_BGR_32F, "BGR format with 32-bit floating-point channels.")
        .value("RG_32F", CMP_FORMAT::CMP_FORMAT_RG_32F, "Two component format with 32-bit floating-point channels.")
        .value("R_32F", CMP_FORMAT::CMP_FORMAT_R_32F, "Single component with 32-bit floating-point channels.")

        // Lossless Based Compression Formats --------------------------------------------------------------------------------
        // Format 0x2nn0
        .value("BROTLIG", CMP_FORMAT::CMP_FORMAT_BROTLIG, "< Lossless CMP format compression : Prototyping")

        // Compression formats ------------ GPU Mapping DirectX, Vulkan and OpenGL formats and comments --------
        // Compressed Format 0xSnn1..0xSnnF   (Keys 0x00Bv..0x00Bv) S =1 is signed, 0 = unsigned, B =Block Compressors 1..7 (BC1..BC7) and v > 1 is a variant like signed or swizzle
        .value("BC1", CMP_FORMAT::CMP_FORMAT_BC1, "DXGI_FORMAT_BC1_UNORM GL_COMPRESSED_RGBA_S3TC_DXT1_EXT A four component opaque (or 1-bit alpha)")
        // compressed texture format for Microsoft DirectX10. Identical to DXT1.  Four bits per pixel.
        .value("BC2", CMP_FORMAT::CMP_FORMAT_BC2, "DXGI_FORMAT_BC2_UNORM VK_FORMAT_BC2_UNORM_BLOCK GL_COMPRESSED_RGBA_S3TC_DXT3_EXT A four component")
        // compressed texture format with explicit alpha for Microsoft DirectX10. Identical to DXT3. Eight bits per pixel.
        .value("BC3", CMP_FORMAT::CMP_FORMAT_BC3, "DXGI_FORMAT_BC3_UNORM VK_FORMAT_BC3_UNORM_BLOCK GL_COMPRESSED_RGBA_S3TC_DXT5_EXT A four component")
        // compressed texture format with interpolated alpha for Microsoft DirectX10. Identical to DXT5. Eight bits per pixel.
        .value("BC4", CMP_FORMAT::CMP_FORMAT_BC4, "DXGI_FORMAT_BC4_UNORM VK_FORMAT_BC4_UNORM_BLOCK GL_COMPRESSED_RED_RGTC1 A single component")
        // compressed texture format for Microsoft DirectX10. Identical to ATI1N. Four bits per pixel.
        .value("BC4_S", CMP_FORMAT::CMP_FORMAT_BC4_S, "DXGI_FORMAT_BC4_SNORM VK_FORMAT_BC4_SNORM_BLOCK GL_COMPRESSED_SIGNED_RED_RGTC1 A single component")
        // compressed texture format for Microsoft DirectX10. Identical to ATI1N. Four bits per pixel.
        .value("BC5", CMP_FORMAT::CMP_FORMAT_BC5, "DXGI_FORMAT_BC5_UNORM VK_FORMAT_BC5_UNORM_BLOCK GL_COMPRESSED_RG_RGTC2 A two component")
        // compressed texture format for Microsoft DirectX10. Identical to ATI2N_XY. Eight bits per pixel.
        .value("BC5_S", CMP_FORMAT::CMP_FORMAT_BC5_S, "DXGI_FORMAT_BC5_SNORM VK_FORMAT_BC5_SNORM_BLOCK GL_COMPRESSED_RGBA_BPTC_UNORM A two component")
        // compressed texture format for Microsoft DirectX10. Identical to ATI2N_XY. Eight bits per pixel.
        .value("BC6H", CMP_FORMAT::CMP_FORMAT_BC6H, "DXGI_FORMAT_BC6H_UF16 VK_FORMAT_BC6H_UFLOAT_BLOCK GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT BC6H compressed texture format (UF)")
        .value("BC6H_SF", CMP_FORMAT::CMP_FORMAT_BC6H_SF, "DXGI_FORMAT_BC6H_SF16 VK_FORMAT_BC6H_SFLOAT_BLOCK GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT   BC6H compressed texture format (SF)")
        .value("BC7", CMP_FORMAT::CMP_FORMAT_BC7, "DXGI_FORMAT_BC7_UNORM VK_FORMAT_BC7_UNORM_BLOCK GL_COMPRESSED_RGBA_BPTC_UNORM BC7  compressed texture format")

        .value("ATI1N", CMP_FORMAT::CMP_FORMAT_ATI1N, "DXGI_FORMAT_BC4_UNORM VK_FORMAT_BC4_UNORM_BLOCK GL_COMPRESSED_RED_RGTC1 Single component")
        // compression format using the same technique as DXT5 alpha. Four bits per pixel.
        .value("ATI2N", CMP_FORMAT::CMP_FORMAT_ATI2N, "DXGI_FORMAT_BC5_UNORM VK_FORMAT_BC5_UNORM_BLOCK GL_COMPRESSED_RG_RGTC2 Two component compression format using the same")
        // technique as DXT5 alpha. Designed for compression of tangent space normal maps. Eight bits per pixel.
        .value("ATI2N_XY", CMP_FORMAT::CMP_FORMAT_ATI2N_XY, "DXGI_FORMAT_BC5_UNORM VK_FORMAT_BC5_UNORM_BLOCK GL_COMPRESSED_RG_RGTC2 Two component compression format using the")
        // same technique as DXT5 alpha. The same as ATI2N but with the channels swizzled. Eight bits per pixel.
        .value("ATI2N_DXT5", CMP_FORMAT::CMP_FORMAT_ATI2N_DXT5, "DXGI_FORMAT_BC5_UNORM VK_FORMAT_BC5_UNORM_BLOCK GL_COMPRESSED_RG_RGTC2 ATI2N like format")
        // using DXT5. Intended for use on GPUs that do not natively support ATI2N. Eight bits per pixel.

        .value("DXT1", CMP_FORMAT::CMP_FORMAT_DXT1, "DXGI_FORMAT_BC1_UNORM VK_FORMAT_BC1_RGB_UNORM_BLOCK GL_COMPRESSED_RGBA_S3TC_DXT1_EXT")
        // A DXTC compressed texture matopaque (or 1-bit alpha). Four bits per pixel.
        .value("DXT3", CMP_FORMAT::CMP_FORMAT_DXT3, "DXGI_FORMAT_BC2_UNORM VK_FORMAT_BC2_UNORM_BLOCK GL_COMPRESSED_RGBA_S3TC_DXT3_EXT")
        // DXTC compressed texture format with explicit alpha. Eight bits per pixel.

        .value("DXT5", CMP_FORMAT::CMP_FORMAT_DXT5, "DXGI_FORMAT_BC3_UNORM VK_FORMAT_BC3_UNORM_BLOCK GL_COMPRESSED_RGBA_S3TC_DXT5_EXT")
        // DXTC compressed texture format with interpolated alpha. Eight bits per pixel.
        .value("DXT5_xGBR", CMP_FORMAT::CMP_FORMAT_DXT5_xGBR, "DXGI_FORMAT_UNKNOWN DXT5 with the red component swizzled into the alpha channel. Eight bits per pixel.")
        .value("DXT5_RxBG", CMP_FORMAT::CMP_FORMAT_DXT5_RxBG, "DXGI_FORMAT_UNKNOWN swizzled DXT5 format with the green component swizzled into the alpha channel. Eight bits per pixel.")
        .value("DXT5_RBxG", CMP_FORMAT::CMP_FORMAT_DXT5_RBxG, "DXGI_FORMAT_UNKNOWN swizzled DXT5 format with the green component swizzled")
        // into the alpha channel & the blue component swizzled into the green channel. Eight bits per pixel.
        .value("DXT5_xRBG", CMP_FORMAT::CMP_FORMAT_DXT5_xRBG, "DXGI_FORMAT_UNKNOWN swizzled DXT5 format with the green component swizzled into")
        // the alpha channel & the red component swizzled into the green channel. Eight bits per pixel.
        .value("DXT5_RGxB", CMP_FORMAT::CMP_FORMAT_DXT5_RGxB, "DXGI_FORMAT_UNKNOWN swizzled DXT5 format with the blue component swizzled into the alpha channel. Eight bits per pixel.")
        .value("DXT5_xGxR", CMP_FORMAT::CMP_FORMAT_DXT5_xGxR, "two-component swizzled DXT5 format with the red component swizzled into the alpha channel &")
        // the green component in the green channel. Eight bits per pixel.

        .value("ATC_RGB", CMP_FORMAT::CMP_FORMAT_ATC_RGB, "CMP - a compressed RGB format.")
        .value("ATC_RGBA_Explicit", CMP_FORMAT::CMP_FORMAT_ATC_RGBA_Explicit, "CMP - a compressed ARGB format with explicit alpha.")
        .value("ATC_RGBA_Interpolated", CMP_FORMAT::CMP_FORMAT_ATC_RGBA_Interpolated, "CMP - a compressed ARGB format with interpolated alpha.")

        .value("ASTC", CMP_FORMAT::CMP_FORMAT_ASTC, "DXGI_FORMAT_UNKNOWN   VK_FORMAT_ASTC_4x4_UNORM_BLOCK to VK_FORMAT_ASTC_12x12_UNORM_BLOCK")
        .value("APC", CMP_FORMAT::CMP_FORMAT_APC, "APC Texture Compressor")
        .value("PVRTC", CMP_FORMAT::CMP_FORMAT_PVRTC, "PVRTC Texture Compressor")

        .value("ETC_RGB", CMP_FORMAT::CMP_FORMAT_ETC_RGB, "DXGI_FORMAT_UNKNOWN VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK GL_COMPRESSED_RGB8_ETC2  backward compatible")
        .value("ETC2_RGB", CMP_FORMAT::CMP_FORMAT_ETC2_RGB, "DXGI_FORMAT_UNKNOWN VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK GL_COMPRESSED_RGB8_ETC2")
        .value("ETC2_SRGB", CMP_FORMAT::CMP_FORMAT_ETC2_SRGB, "DXGI_FORMAT_UNKNOWN VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK GL_COMPRESSED_SRGB8_ETC2")
        .value("ETC2_RGBA", CMP_FORMAT::CMP_FORMAT_ETC2_RGBA, "DXGI_FORMAT_UNKNOWN VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK GL_COMPRESSED_RGBA8_ETC2_EAC")
        .value("ETC2_RGBA1", CMP_FORMAT::CMP_FORMAT_ETC2_RGBA1, "DXGI_FORMAT_UNKNOWN VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2")
        .value("ETC2_SRGBA", CMP_FORMAT::CMP_FORMAT_ETC2_SRGBA, "DXGI_FORMAT_UNKNOWN VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC")
        .value("ETC2_SRGBA1", CMP_FORMAT::CMP_FORMAT_ETC2_SRGBA1, "DXGI_FORMAT_UNKNOWN VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2")

        // New Compression Formats -------------------------------------------------------------
        .value("BINARY", CMP_FORMAT::CMP_FORMAT_BINARY, "< Binary/Raw Data Format")
        .value("GTC", CMP_FORMAT::CMP_FORMAT_GTC, "< GTC   Fast Gradient Texture Compressor")
        .value("BASIS", CMP_FORMAT::CMP_FORMAT_BASIS, "< BASIS compression")

        .value("MAX", CMP_FORMAT::CMP_FORMAT_MAX, "Invalid Format");

    nb::enum_<CMP_Compute_type>(m, "CMP_ComputeType")
        .value("UNKNOWN", CMP_Compute_type::CMP_UNKNOWN)
        .value("CPU", CMP_Compute_type::CMP_CPU, "Use CPU Only, encoders defined CMP_CPUEncode or Compressonator lib will be used")
        .value("HPC", CMP_Compute_type::CMP_HPC, "Use CPU High Performance Compute Encoders with SPMD support defined in CMP_CPUEncode")
        .value("GPU_OCL", CMP_Compute_type::CMP_GPU_OCL, "Use GPU Kernel Encoders to compress textures using OpenCL Framework")
        .value("GPU_DXC", CMP_Compute_type::CMP_GPU_DXC, "Use GPU Kernel Encoders to compress textures using DirectX Compute Framework")
        .value("GPU_VLK", CMP_Compute_type::CMP_GPU_VLK, "Use GPU Kernel Encoders to compress textures using Vulkan Compute Framework")
        .value("GPU_HW", CMP_Compute_type::CMP_GPU_HW, "Use GPU HW to encode textures , using gl extensions");

    nb::enum_<CMP_Speed>(m, "CMP_Speed")
        .value("Normal", CMP_Speed::CMP_Speed_Normal, "Highest quality mode")
        .value("Fast", CMP_Speed::CMP_Speed_Fast, "Slightly lower quality but much faster compression mode - DXTn & ATInN only")
        .value("SuperFast", CMP_Speed::CMP_Speed_SuperFast, "Slightly lower quality but much, much faster compression mode - DXTn & ATInN only");

    nb::enum_<CMP_GPUDecode>(m, "CMP_GPUDecode")
        .value("OPENGL", CMP_GPUDecode::GPUDecode_OPENGL, "Use OpenGL to decode Textures (default)")
        .value("DIRECTX", CMP_GPUDecode::GPUDecode_DIRECTX, "Use DirectX to decode Textures")
        .value("VULKAN", CMP_GPUDecode::GPUDecode_VULKAN, "Use Vulkan to decode Textures")
        .value("INVALID", CMP_GPUDecode::GPUDecode_INVALID);

    nb::enum_<CMP_ChannelFormat>(m, "CMP_ChannelFormat")
        .value("_8bit", CMP_ChannelFormat::CF_8bit, "8-bit integer data.")
        .value("Float16", CMP_ChannelFormat::CF_Float16, "16-bit float data.")
        .value("Float32", CMP_ChannelFormat::CF_Float32, "32-bit float data.")
        .value("Compressed", CMP_ChannelFormat::CF_Compressed, "Compressed data.")
        .value("_16bit", CMP_ChannelFormat::CF_16bit, "16-bit integer data.")
        .value("_2101010", CMP_ChannelFormat::CF_2101010, "10-bit integer data in the color channels & 2-bit integer data in the alpha channel.")
        .value("_32bit", CMP_ChannelFormat::CF_32bit, "32-bit integer data.")
        .value("Float9995E", CMP_ChannelFormat::CF_Float9995E, "32-bit partial precision float.")
        .value("YUV_420", CMP_ChannelFormat::CF_YUV_420, "YUV Chroma formats")
        .value("YUV_422", CMP_ChannelFormat::CF_YUV_422, "YUV Chroma formats")
        .value("YUV_444", CMP_ChannelFormat::CF_YUV_444, "YUV Chroma formats")
        .value("YUV_4444", CMP_ChannelFormat::CF_YUV_4444, "YUV Chroma formats")
        .value("_1010102", CMP_ChannelFormat::CF_1010102, "");

    nb::enum_<CMP_TextureDataType>(m, "CMP_TextureDataType")
        .value("XRGB", CMP_TextureDataType::TDT_XRGB, "An RGB texture padded to DWORD width.")
        .value("ARGB", CMP_TextureDataType::TDT_ARGB, "An ARGB texture.")
        .value("NORMAL_MAP", CMP_TextureDataType::TDT_NORMAL_MAP, "A normal map.")
        .value("R", CMP_TextureDataType::TDT_R, "A single component texture.")
        .value("RG", CMP_TextureDataType::TDT_RG, "A two component texture.")
        .value("YUV_SD", CMP_TextureDataType::TDT_YUV_SD, "An YUB Standard Definition texture.")
        .value("YUV_HD", CMP_TextureDataType::TDT_YUV_HD, "An YUB High Definition texture.")
        .value("RGB", CMP_TextureDataType::TDT_RGB, "An RGB texture")
        .value("_8", CMP_TextureDataType::TDT_8, "8  Bit untyped data")
        .value("_16", CMP_TextureDataType::TDT_16, "16 Bit untyped data");

    nb::enum_<CMP_TextureType>(m, "CMP_TextureType")
        .value("_2D", CMP_TextureType::TT_2D, "A regular 2D texture. data stored linearly (rgba,rgba,...rgba)")
        .value("CubeMap", CMP_TextureType::TT_CubeMap, "A cubemap texture.")
        .value("VolumeTexture", CMP_TextureType::TT_VolumeTexture, "A volume texture.")
        .value("_2D_Block", CMP_TextureType::TT_2D_Block, "2D texture data stored as [Height][Width] blocks as individual channels using cmp_rgb_t or cmp_yuv_t")
        .value("_1D", CMP_TextureType::TT_1D, "Untyped data stored linearly")
        .value("Unknown", CMP_TextureType::TT_Unknown, "Unknown type of texture : No data is stored for this type");

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //      Classes
    //
    //////////////////////////////////////////////////////////////////////////////////////////

    nb::class_<AMD_CMD_SET>(m, "AMD_CMD_Set")
        .def("__init__", [](AMD_CMD_SET *acs, const std::string &strCommand, const std::string &strParameter)
             {
        // 1. Validate inputs
        if (strCommand.length() >= AMD_MAX_CMD_STR) {
            throw nb::value_error("Command length too long!");
        }
        if (strParameter.length() >= AMD_MAX_CMD_PARAM) {
            throw nb::value_error("Parameter length too long!");
        }

        // 3. Populate C-style array fields safely
        memset(acs->strCommand, 0, AMD_MAX_CMD_STR);
        memcpy(acs->strCommand, strCommand.data(), strCommand.length());

        memset(acs->strParameter, 0, AMD_MAX_CMD_PARAM);
        memcpy(acs->strParameter, strParameter.data(), strParameter.length()); }, nb::arg("strCommand"), nb::arg("strParameter"))
        .def_ro("strCommand", &AMD_CMD_SET::strCommand)
        .def_ro("strParameter", &AMD_CMD_SET::strParameter);

    nb::class_<KernelPerformanceStats>(m, "KernelPerformanceStats")
        .def_ro("m_computeShaderElapsedMS", &KernelPerformanceStats::m_computeShaderElapsedMS)
        .def_ro("m_num_blocks", &KernelPerformanceStats::m_num_blocks)
        .def_ro("m_CmpMTxPerSec", &KernelPerformanceStats::m_CmpMTxPerSec);

    nb::class_<KernelDeviceInfo>(m, "KernelDeviceInfo")
        .def_ro("m_deviceName", &KernelDeviceInfo::m_deviceName)
        .def_ro("m_version", &KernelDeviceInfo::m_version)
        .def_ro("m_maxUCores", &KernelDeviceInfo::m_maxUCores);

    nb::class_<CMP_CompressOptions>(m, "CMP_CompressOptions")
        .def(nb::new_([]()
                      {
            auto options = new CMP_CompressOptions();
            memset(options, 0, sizeof(CMP_CompressOptions));
            options->dwSize = sizeof(CMP_CompressOptions);
            return options; }))
        // New to v4.5
        // Flags to control parameters in Brotli-G compression preconditioning
        .def_rw("doPreconditionBRLG", &CMP_CompressOptions::doPreconditionBRLG, "")
        .def_rw("doDeltaEncodeBRLG", &CMP_CompressOptions::doDeltaEncodeBRLG, "")
        .def_rw("doSwizzleBRLG", &CMP_CompressOptions::doSwizzleBRLG, "")
        // New to v4.3
        .def_rw("dwPageSize", &CMP_CompressOptions::dwPageSize, "Used by Brotli-G Codec for setting the page size used for compression")
        // New to v4.2
        .def_rw("bUseRefinementSteps", &CMP_CompressOptions::bUseRefinementSteps, "Used by BC1, BC2, and BC3 codecs to improve quality, this setting will increase encoding time for better quality results")
        .def_rw("nRefinementSteps", &CMP_CompressOptions::nRefinementSteps, "Currently only 1 step is implemented")
        // v4.1 and older settings
        .def_rw("bUseChannelWeighting", &CMP_CompressOptions::bUseChannelWeighting, "Use channel weights. With swizzled formats the weighting applies to the data within the specified channel not the channel itself. Channel weigthing is not implemented for BC6H and BC7")
        .def_rw("fWeightingRed", &CMP_CompressOptions::fWeightingRed, "The weighting of the Red or X Channel.")
        .def_rw("fWeightingGreen", &CMP_CompressOptions::fWeightingGreen, "The weighting of the Green or Y Channel.")
        .def_rw("fWeightingBlue", &CMP_CompressOptions::fWeightingBlue, "The weighting of the Blue or Z Channel.")
        .def_rw("bUseAdaptiveWeighting", &CMP_CompressOptions::bUseAdaptiveWeighting, "Adapt weighting on a per-block basis.")
        .def_rw("bDXT1UseAlpha", &CMP_CompressOptions::bDXT1UseAlpha, "Encode single-bit alpha data. Only valid when compressing to DXT1 & BC1.")
        .def_rw("bUseGPUDecompress", &CMP_CompressOptions::bUseGPUDecompress, "Use GPU to decompress. Decode API can be changed by specified in DecodeWith parameter. Default is OpenGL.")
        .def_rw("bUseCGCompress", &CMP_CompressOptions::bUseCGCompress, "Use SPMD/GPU to compress. Encode API can be changed by specified in EncodeWith parameter. Default is OpenCL.")
        .def_rw("nAlphaThreshold", &CMP_CompressOptions::nAlphaThreshold, "The alpha threshold to use when compressing to DXT1 & BC1 with bDXT1UseAlpha.")
        // Texels with an alpha value less than the threshold are treated as transparent.
        // Note: When nCompressionSpeed is not set to Normal AphaThreshold is ignored for DXT1 & BC1
        .def_rw("bDisableMultiThreading", &CMP_CompressOptions::bDisableMultiThreading, "Disable multi-threading of the compression. This will slow the compression but can be")
        // useful if you're managing threads in your application.
        // if set BC7 dwnumThreads will default to 1 during encoding and then return back to its original value when done.
        .def_rw("nCompressionSpeed", &CMP_CompressOptions::nCompressionSpeed, "The trade-off between compression speed & quality.")
        // Notes:
        // 1. This value is ignored for BC6H and BC7 (for BC7 the compression speed depends on fquaility value)
        // 2. For 64 bit DXT1 to DXT5 and BC1 to BC5 nCompressionSpeed is ignored and set to Noramal Speed
        // 3. To force the use of nCompressionSpeed setting regarless of Note 2 use fQuality at 0.05
        .def_rw("nGPUDecode", &CMP_CompressOptions::nGPUDecode, "This value is set using DecodeWith argument (OpenGL, DirectX) default is OpenGL")
        .def_rw("nEncodeWith", &CMP_CompressOptions::nEncodeWith, "This value is set using EncodeWith argument, currently only OpenCL is used")
        .def_rw("dwnumThreads", &CMP_CompressOptions::dwnumThreads, "Number of threads to initialize for BC7 encoding (Max up to 128). Default set to auto,")
        .def_rw("fquality", &CMP_CompressOptions::fquality, "Quality of encoding. This value ranges between 0.0 and 1.0. BC7 & BC6 default is 0.05, others codecs are set at 1.0")
        // setting fquality above 0.0 gives the fastest, lowest quality encoding, 1.0 is the slowest,
        // highest quality encoding. Default set to a low value of 0.05
        .def_rw("brestrictColour", &CMP_CompressOptions::brestrictColour, "This setting is a quality tuning setting for BC7 which may be necessary for convenience in some")
        // applications. Default set to false. If set and the block does not need alpha it instructs
        //  the code not to use modes that have combined colour + alpha - this avoids the possibility that the encoder might
        //  choose an alpha other than 1.0 (due to parity) and cause something to become accidentally slightly transparent
        //  (it's possible that when encoding 3-component texture applications will assume that the 4th component can
        //  safely be assumed to be 1.0 all the time.)
        .def_rw("brestrictAlpha", &CMP_CompressOptions::brestrictAlpha, "This setting is a quality tuning setting for BC7 which may be necessary for some textures. Default set to false,")
        // if set it will also apply restriction to blocks with alpha to avoid issues with punch-through
        // or thresholded alpha encoding
        .def_rw("dwmodeMask", &CMP_CompressOptions::dwmodeMask, "Mode to set BC7 to encode blocks using any of 8 different block modes in order to obtain the highest quality. Default set to 0xFF)")
        // You can combine the bits to test for which modes produce the best image quality.
        // The mode that produces the best image quality above a set quality level (fquality) is used and subsequent modes set in the mask
        // are not tested, this optimizes the performance of the compression versus the required quality.
        // If you prefer to check all modes regardless of the quality then set the fquality to a value of 0
        // .def_rw("NumCmds", &CMP_CompressOptions::NumCmds, "Count of the number of command value pairs in CmdSet[].  Max value that can be set is AMD_MAX_CMDS = 20 on this release")
        // .def_rw("CmdSet", &CMP_CompressOptions::CmdSet, "Extended command options that can be set for the specified codec")
        .def_prop_rw(
            "CmdSet",
            // Getter: Returns a vector/list of active commands up to NumCmds (or the full array)
            [](const CMP_CompressOptions &self) -> std::vector<AMD_CMD_SET>
            {
                size_t count = std::min<size_t>(self.NumCmds, 20);
                return std::vector<AMD_CMD_SET>(self.CmdSet, self.CmdSet + count);
            },
            [](CMP_CompressOptions &self, const std::vector<AMD_CMD_SET> &cmds)
            {
                if (cmds.size() > 20)
                {
                    throw std::invalid_argument("CmdSet size exceeds AMD_MAX_CMDS limit of 20");
                }
                for (size_t i = 0; i < cmds.size(); ++i)
                {
                    self.CmdSet[i] = cmds[i];
                }
                self.NumCmds = static_cast<CMP_INT>(cmds.size());
            },
            "Extended command options that can be set for the specified codec")
        .def_rw("fInputDefog", &CMP_CompressOptions::fInputDefog, "ToneMap properties for float type image send into non float compress algorithm.")
        .def_rw("fInputExposure", &CMP_CompressOptions::fInputExposure, "")
        .def_rw("fInputKneeLow", &CMP_CompressOptions::fInputKneeLow, "")
        .def_rw("fInputKneeHigh", &CMP_CompressOptions::fInputKneeHigh, "")
        .def_rw("fInputGamma", &CMP_CompressOptions::fInputGamma, "")
        .def_rw("fInputFilterGamma", &CMP_CompressOptions::fInputFilterGamma, "Gamma correction value applied for mipmap generation")

        .def_rw("iCmpLevel", &CMP_CompressOptions::iCmpLevel, "< draco setting: compression level (range 0-10: higher mean more compressed) - default 7")
        .def_rw("iPosBits", &CMP_CompressOptions::iPosBits, "quantization bits for position - default 14")
        .def_rw("iTexCBits", &CMP_CompressOptions::iTexCBits, "quantization bits for texture coordinates - default 12")
        .def_rw("iNormalBits", &CMP_CompressOptions::iNormalBits, "quantization bits for normal - default 10")
        .def_rw("iGenericBits", &CMP_CompressOptions::iGenericBits, "quantization bits for generic - default 8")

#ifdef USE_3DMESH_OPTIMIZE
        .def_rw("iVcacheSize", &CMP_CompressOptions::iVcacheSize, "For mesh vertices optimization, hardware vertex cache size. (value range 1 - no limit as it")
        // allows users to simulate hardware cache size to find the most optimum size)- default is enabled with cache size = 16
        .def_rw("iVcacheFIFOSize", &CMP_CompressOptions::iVcacheFIFOSize, "For mesh vertices optimization, hardware vertex cache size. (value range 1 - no limit as it")
        // allows users to simulate hardware cache size to find the most optimum size)- default is disabled.
        .def_rw("fOverdrawACMR", &CMP_CompressOptions::fOverdrawACMR, "For mesh overdraw optimization,  optimize overdraw with ACMR (average cache miss ratio)")
        // threshold value specified (value range 1-3) - default is enabled with ACMR value = 1.05 (i.e. 5% worse)
        .def_rw("iSimplifyLOD", &CMP_CompressOptions::iSimplifyLOD, "simplify mesh using LOD (Level of Details) value specified.(value range 1- no limit as it allows users")
        // to simplify the mesh until the level they desired. Higher level means less triangles drawn, less details.)
        .def_rw("bVertexFetch", &CMP_CompressOptions::bVertexFetch, "optimize vertices fetch . boolean value 0 - disabled, 1-enabled. -default is enabled.")
#endif

        .def_rw("SourceFormat", &CMP_CompressOptions::SourceFormat, "")
        .def_rw("DestFormat", &CMP_CompressOptions::DestFormat, "")
        .def_rw("format_support_hostEncoder", &CMP_CompressOptions::format_support_hostEncoder, "Temp setting used while encoding with gpu or hpc plugins")

        // User Print Info interface
        // .def_prop_rw("m_PrintInfoStr", [](const CMP_CompressOptions *self) -> std::function<void(const char *)>
        //              {
        //     std::lock_guard<std::mutex> guard(PrintInfoTrampoline::lock);
        //     auto it = PrintInfoTrampoline::registry.find(self);
        //     return (it != PrintInfoTrampoline::registry.end()) ? it->second : nullptr; }, [](CMP_CompressOptions &self, std::function<void(const char *)> cb)
        //              {
        //     std::lock_guard<std::mutex> guard(PrintInfoTrampoline::lock);
        //     if (cb) {
        //         // Store the std::function persistently in C++ memory so it isn't garbage collected
        //         PrintInfoTrampoline::registry[self] = cb;
        //         self.m_PrintInfoStr = &PrintInfoTrampoline::c_callback;
        //     } else {
        //         PrintInfoTrampoline::registry.erase(self);
        //         self.m_PrintInfoStr = nullptr;
        //     } }, "A callback function to print out information from the encoder.")
        // User Info for Performance Query on GPU or CPU Encoder Processing
        .def_rw("getPerfStats", &CMP_CompressOptions::getPerfStats, "Set to true if you want to get Performance Stats")
        .def_rw("perfStats", &CMP_CompressOptions::perfStats, "Data storage for the performance stats obtained from GPU or CPU while running encoder processing")
        .def_rw("getDeviceInfo", &CMP_CompressOptions::getDeviceInfo, "Set to true if you want to get target device info")
        .def_rw("deviceInfo", &CMP_CompressOptions::deviceInfo, "Data storage for the performance stats obtained from GPU or CPU while running encoder processing")
        .def_rw("genGPUMipMaps", &CMP_CompressOptions::genGPUMipMaps, "When ecoding with GPU HW use it to generate MipMap images, valid only when miplevels is set else default is toplevel 1")
        .def_rw("useSRGBFrames", &CMP_CompressOptions::useSRGBFrames, "when using GPU HW for encoding and mipmap generation use SRGB frames, default is RGB")
        .def_rw("miplevels", &CMP_CompressOptions::miplevels, "miplevels to use when GPU is used to generate them");

    static const CMP_CompressOptions default_options = []()
    {
        CMP_CompressOptions opts{0};
        opts.dwSize = sizeof(CMP_CompressOptions);
        opts.fquality = 0.05f;
        return opts;
    }();

    nb::bind_vector<std::vector<uint8_t>>(m, "Vec<U8>");
    nb::bind_vector<std::vector<AMD_CMD_SET>>(m, "Vec<AMD_CMD_SET>");

    nb::class_<CMP_Texture_Wrapper>(m, "CMP_Texture")
        .def(
            nb::init<CMP_DWORD, CMP_DWORD, CMP_DWORD, CMP_FORMAT, nb::object, CMP_FORMAT, CMP_DWORD, CMP_DWORD, CMP_DWORD>(),
            nb::arg("dwWidth"),
            nb::arg("dwHeight"),
            nb::arg("dwPitch"),
            nb::arg("format"),
            nb::arg("dwData") = nb::none(),
            nb::arg("transcodeFormat") = CMP_FORMAT::CMP_FORMAT_MAX,
            nb::arg("nBlockHeight") = 4,
            nb::arg("nBlockWidth") = 4,
            nb::arg("nBlockDepth") = 1)
        .def("convert", &CMP_Texture_Wrapper::convert, nb::arg("other"), nb::arg("options") = default_options, nb::arg("feedback") = nb::none())
        .def_rw("dwData", &CMP_Texture_Wrapper::storage, "Raw texture data buffer (read/write)")
        .def_prop_ro("dwWidth", [](const CMP_Texture_Wrapper &w)
                     { return w.inner.dwWidth; })
        .def_prop_ro("dwHeight", [](const CMP_Texture_Wrapper &w)
                     { return w.inner.dwHeight; })
        .def_prop_ro("dwPitch", [](const CMP_Texture_Wrapper &w)
                     { return w.inner.dwPitch; })
        .def_prop_ro("format", [](const CMP_Texture_Wrapper &w)
                     { return w.inner.format; })
        .def_prop_ro("transcodeFormat", [](const CMP_Texture_Wrapper &w)
                     { return w.inner.transcodeFormat; })
        .def_prop_ro("nBlockHeight", [](const CMP_Texture_Wrapper &w)
                     { return w.inner.nBlockHeight; })
        .def_prop_ro("nBlockWidth", [](const CMP_Texture_Wrapper &w)
                     { return w.inner.nBlockWidth; })
        .def_prop_ro("nBlockDepth", [](const CMP_Texture_Wrapper &w)
                     { return w.inner.nBlockDepth; })
        .def_prop_ro("dwDataSize", [](const CMP_Texture_Wrapper &w)
                     { return w.inner.dwDataSize; })
        .def("__eq__", [](const CMP_Texture_Wrapper &a, const CMP_Texture_Wrapper &b)
             { return a == b; })
        .def("__ne__", [](const CMP_Texture_Wrapper &a, const CMP_Texture_Wrapper &b)
             { return !(a == b); })
        .def("__repr__", [](const CMP_Texture_Wrapper &self)
             {
    std::ostringstream oss;
    oss << "CMP_Texture("
        << "width=" << self.inner.dwWidth << ", "
        << "height=" << self.inner.dwHeight << ", "
        << "format=" <<  self.inner.format
        << ")";
    return oss.str(); })
        .def("view", [](CMP_Texture_Wrapper &w)
             {
        std::vector<size_t> shape;

        auto channelInfo = getChannelFormatInfo(w.inner.format);
        const auto &channels = channelInfo.channels;
        const auto &packed = channelInfo.packed;

        if (channels == 0 && !packed)
        {
            // Case 1: Compressed, Packed, etc -> direct buffer
            shape = {static_cast<size_t>(w.inner.dwDataSize)};
        }
        else if (channelInfo.channels == 1 || packed)
        {
            // Case 2: 2D Grayscale or Single-scalar packed formats (H, W)
            shape = {
                static_cast<size_t>(w.inner.dwHeight),
                static_cast<size_t>(w.inner.dwWidth)};
        }
        else
        {
            // Case 3: Standard multi-channel formats (H, W, C)
            shape = {
                static_cast<size_t>(w.inner.dwHeight),
                static_cast<size_t>(w.inner.dwWidth),
                static_cast<size_t>(channelInfo.channels)};
        }

        return nb::ndarray<nb::array_api, nb::c_contig>(
            w.data(),
            shape.size(),
            shape.data(),
            /* owner = */ nb::find(w),
            /* strides = */ nullptr, // nanobind automatically computes C-contiguous strides
            /* dtype = */ channelInfo.dtype); });
}