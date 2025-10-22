# =====================================================================
#  Copyright (c) 2007-2024    Advanced Micro Devices, Inc. All rights reserved.
#  Copyright (c) 2004-2006    ATI Technologies Inc.
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files(the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions :
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#  THE SOFTWARE.
#
#  \file Compressonator.h
#
# =====================================================================
from enum import IntEnum


class CMP_Format(IntEnum):
    """
    Texture format.

    Formating
    ---------
        Key to format types 0xFnbC

        C = 0 is uncompressed
        C > 0 is compressed

        For C = 0 uncompressed
            F = 0 is Byte data,
            F = 1 is Float data
            nb is a format type

        For C >= 1 Compressed
            F = 0 is unsigned data
            F = 1 is signed data
            For C = 1 BCn
                b = format is a BCn block comprerssor where b is 1..7 for BC1..BC7
            For C > 1
                is a varaiant of the format (example: swizzled format for DXTC, or a signed version)


    Attributes
    ----------
    RGBA_8888_S: int
        RGBA format with signed 8-bit fixed channels.
    ARGB_8888_S: int
        ARGB format with signed 8-bit fixed channels.
    ARGB_8888: int
        ARGB format with 8-bit fixed channels.
    ABGR_8888: int
        ABGR format with 8-bit fixed channels.
    RGBA_8888: int
        RGBA format with 8-bit fixed channels.
    BGRA_8888: int
        BGRA format with 8-bit fixed channels.
    RGB_888: int
        RGB format with 8-bit fixed channels.
    RGB_888_S: int
        RGB format with 8-bit fixed channels.
    BGR_888: int
        BGR format with 8-bit fixed channels.
    RG_8_S: int
        Two component format with signed 8-bit fixed channels.
    RG_8: int
        Two component format with 8-bit fixed channels.
    R_8_S: int
        Single component format with signed 8-bit fixed channel.
    R_8: int
        Single component format with 8-bit fixed channel.
    ARGB_2101010: int
        ARGB format with 10-bit fixed channels for color & a 2-bit fixed channel for alpha.
    RGBA_1010102: int
        RGBA format with 10-bit fixed channels for color & a 2-bit fixed channel for alpha.
    ARGB_16: int
        ARGB format with 16-bit fixed channels.
    ABGR_16: int
        ABGR format with 16-bit fixed channels.
    RGBA_16: int
        RGBA format with 16-bit fixed channels.
    BGRA_16: int
        BGRA format with 16-bit fixed channels.
    RG_16: int
        Two component format with 16-bit fixed channels.
    R_16: int
        Single component format with 16-bit fixed channels.
    RGBE_32F: int
        RGB format with 9-bit floating point each channel and shared 5 bit exponent
    ARGB_16F: int
        ARGB format with 16-bit floating-point channels.
    ABGR_16F: int
        ABGR format with 16-bit floating-point channels.
    RGBA_16F: int
        RGBA format with 16-bit floating-point channels.
    BGRA_16F: int
        BGRA format with 16-bit floating-point channels.
    RG_16F: int
        Two component format with 16-bit floating-point channels.
    R_16F: int
        Single component with 16-bit floating-point channels.
    ARGB_32F: int
        ARGB format with 32-bit floating-point channels.
    ABGR_32F: int
        ABGR format with 32-bit floating-point channels.
    RGBA_32F: int
        RGBA format with 32-bit floating-point channels.
    BGRA_32F: int
        BGRA format with 32-bit floating-point channels.
    RGB_32F: int
        RGB format with 32-bit floating-point channels.
    BGR_32F: int
        BGR format with 32-bit floating-point channels.
    RG_32F: int
        Two component format with 32-bit floating-point channels.
    R_32F: int
        Single component with 32-bit floating-point channels.
    BROTLIG: int
        Lossless CMP format compression : Prototyping
    BC1: int
        DXGI_FORMAT_BC1_UNORM GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
        A four component opaque (or 1-bit alpha) compressed texture format for Microsoft DirectX10.
        Identical to DXT1.
        Four bits per pixel.
    BC2: int
        DXGI_FORMAT_BC2_UNORM
        VK_FORMAT_BC2_UNORM_BLOCK
        GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
        A four component compressed texture format with explicit alpha for Microsoft DirectX10.
        Identical to DXT3.
        Eight bits per pixel.
    BC3: int
        DXGI_FORMAT_BC3_UNORM
        VK_FORMAT_BC3_UNORM_BLOCK
        GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
        A four component compressed texture format with interpolated alpha for Microsoft DirectX10.
        Identical to DXT5. Eight bits per pixel.
    BC4: int
        DXGI_FORMAT_BC4_UNORM
        VK_FORMAT_BC4_UNORM_BLOCK
        GL_COMPRESSED_RED_RGTC1
        A single component compressed texture format for Microsoft DirectX10.
        Identical to ATI1N. Four bits per pixel.
    BC4_S: int
        DXGI_FORMAT_BC4_SNORM
        VK_FORMAT_BC4_SNORM_BLOCK
        GL_COMPRESSED_SIGNED_RED_RGTC1
        A single component compressed texture format for Microsoft DirectX10.
        Identical to ATI1N. Four bits per pixel.
    BC5: int
        DXGI_FORMAT_BC5_UNORM
        VK_FORMAT_BC5_UNORM_BLOCK
        GL_COMPRESSED_RG_RGTC2
        A two component compressed texture format for Microsoft DirectX10.
        Identical to ATI2N_XY. Eight bits per pixel.
    BC5_S: int
        DXGI_FORMAT_BC5_SNORM
        VK_FORMAT_BC5_SNORM_BLOCK
        GL_COMPRESSED_RGBA_BPTC_UNORM
        A two component compressed texture format for Microsoft DirectX10.
        Identical to ATI2N_XY. Eight bits per pixel.
    BC6H: int
        DXGI_FORMAT_BC6H_UF16
        VK_FORMAT_BC6H_UFLOAT_BLOCK
        GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
        BC6H compressed texture format (UF)
    BC6H_SF: int
        DXGI_FORMAT_BC6H_SF16
        VK_FORMAT_BC6H_SFLOAT_BLOCK
        GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
        BC6H compressed texture format (SF)
    BC7: int
        DXGI_FORMAT_BC7_UNORM
        VK_FORMAT_BC7_UNORM_BLOCK
        GL_COMPRESSED_RGBA_BPTC_UNORM
        BC7  compressed texture format
    ATI1N: int
        DXGI_FORMAT_BC4_UNORM
        VK_FORMAT_BC4_UNORM_BLOCK
        GL_COMPRESSED_RED_RGTC1
        Single component compression format using the same technique as DXT5 alpha.
        Four bits per pixel.
    ATI2N: int
        DXGI_FORMAT_BC5_UNORM
        VK_FORMAT_BC5_UNORM_BLOCK
        GL_COMPRESSED_RG_RGTC2
        Two component compression format using the same technique as DXT5 alpha.
        Designed for compression of tangent space normal maps.
        Eight bits per pixel.
    ATI2N_XY: int
        DXGI_FORMAT_BC5_UNORM
        VK_FORMAT_BC5_UNORM_BLOCK
        GL_COMPRESSED_RG_RGTC2
        Two component compression format using the same technique as DXT5 alpha.
        The same as ATI2N but with the channels swizzled.
        Eight bits per pixel.
    ATI2N_DXT5: int
        DXGI_FORMAT_BC5_UNORM
        VK_FORMAT_BC5_UNORM_BLOCK
        GL_COMPRESSED_RG_RGTC2
        ATI2N like format using DXT5.
        Intended for use on GPUs that do not natively support
        ATI2N. Eight bits per pixel.
    DXT1: int
        DXGI_FORMAT_BC1_UNORM
        VK_FORMAT_BC1_RGB_UNORM_BLOCK
        GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
        A DXTC compressed texture matopaque (or 1-bit alpha).
        Four bits per pixel.
    DXT3: int
        DXGI_FORMAT_BC2_UNORM
        VK_FORMAT_BC2_UNORM_BLOCK
        GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
        DXTC compressed texture format with explicit alpha.
        Eight bits per pixel.
    DXT5: int
        DXGI_FORMAT_BC3_UNORM
        VK_FORMAT_BC3_UNORM_BLOCK
        GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
        DXTC compressed texture format with interpolated alpha.
        Eight bits per pixel.
    DXT5_xGBR: int
        DXGI_FORMAT_UNKNOWN
        DXT5 with the red component swizzled into the alpha channel.
        Eight bits per pixel.
    DXT5_RxBG: int
        DXGI_FORMAT_UNKNOWN
        swizzled DXT5 format with the green component swizzled into the alpha channel.
        Eight bits per pixel.
    DXT5_RBxG: int
        DXGI_FORMAT_UNKNOWN
        swizzled DXT5 format with the green component swizzled into the alpha channel & the blue component swizzled into the green channel.
        Eight bits per pixel.
    DXT5_xRBG: int
        DXGI_FORMAT_UNKNOWN swizzled DXT5 format with the green component swizzled into the alpha channel & the component swizzled into the green channel.
        Eight bits per pixel.
    DXT5_RGxB: int
        DXGI_FORMAT_UNKNOWN
        swizzled DXT5 format with the blue component swizzled into the alpha channel.
        Eight bits per pixel.
    DXT5_xGxR: int
        two-component swizzled DXT5 format with the red component swizzled into the alpha channel & the green component in the green channel.
        Eight bits per pixel.
    ATC_RGB: int
        CMP - a compressed RGB format.
    ATC_RGBA_Explicit: int
        CMP - a compressed ARGB format with explicit alpha.
    ATC_RGBA_Interpolated: int
        CMP - a compressed ARGB format with interpolated alpha.
    ASTC: int
        DXGI_FORMAT_UNKNOWN
        VK_FORMAT_ASTC_4x4_UNORM_BLOCK to VK_FORMAT_ASTC_12x12_UNORM_BLOCK
    APC: int
        APC Texture Compressor
    PVRTC: int
        PowerVR Texture Compressor
    ETC_RGB: int
        DXGI_FORMAT_UNKNOWN
        VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK
        GL_COMPRESSED_RGB8_ETC2
        backward compatible
    ETC2_RGB: int
        DXGI_FORMAT_UNKNOWN
        VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK
        GL_COMPRESSED_RGB8_ETC2
    ETC2_SRGB: int
        DXGI_FORMAT_UNKNOWN
        VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK
        GL_COMPRESSED_SRGB8_ETC2
    ETC2_RGBA: int
        DXGI_FORMAT_UNKNOWN
        VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK
        GL_COMPRESSED_RGBA8_ETC2_EAC
    ETC2_RGBA1: int
        DXGI_FORMAT_UNKNOWN
        VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK
        GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
    ETC2_SRGBA: int
        DXGI_FORMAT_UNKNOWN
        VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK
        GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
    ETC2_SRGBA1: int
        DXGI_FORMAT_UNKNOWN
        VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK
        GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2
    BINARY: int
        Binary/Raw Data Format
    GTC: int
        GTC Fast Gradient Texture Compressor
    BASIS: int
        BASIS compression
    MAX: int
        Invalid Format
    """

    Unknown = 0x0000
    # Channel Component formats
    # --------------------------------------------------------------------------------
    # Byte Format 0x0nn0
    RGBA_8888_S = 0x0010
    ARGB_8888_S = 0x0020
    ARGB_8888 = 0x0030
    ABGR_8888 = 0x0040
    RGBA_8888 = 0x0050
    BGRA_8888 = 0x0060
    RGB_888 = 0x0070
    RGB_888_S = 0x0080
    BGR_888 = 0x0090
    RG_8_S = 0x00A0
    RG_8 = 0x00B0
    R_8_S = 0x00C0
    R_8 = 0x00D0
    ARGB_2101010 = 0x00E0
    RGBA_1010102 = 0x00F0
    ARGB_16 = 0x0100
    ABGR_16 = 0x0110
    RGBA_16 = 0x0120
    BGRA_16 = 0x0130
    RG_16 = 0x0140
    R_16 = 0x0150
    RGBE_32F = 0x1000
    ARGB_16F = 0x1010
    ABGR_16F = 0x1020
    RGBA_16F = 0x1030
    BGRA_16F = 0x1040
    RG_16F = 0x1050
    R_16F = 0x1060
    ARGB_32F = 0x1070
    ABGR_32F = 0x1080
    RGBA_32F = 0x1090
    BGRA_32F = 0x10A0
    RGB_32F = 0x10B0
    BGR_32F = 0x10C0
    RG_32F = 0x10D0
    R_32F = 0x10E0
    # Lossless Based Compression Formats
    # --------------------------------------------------------------------------------
    # Format 0x2nn0
    BROTLIG = 0x2000

    # Compression formats
    # ------------ GPU Mapping DirectX, Vulkan and OpenGL formats and comments --------
    # Compressed Format 0xSnn1..0xSnnF   (Keys 0x00Bv..0x00Bv)
    # S =1 is signed, 0 = unsigned
    # B =Block Compressors 1..7 (BC1..BC7)
    # v > 1 is a variant like signed or swizzle
    BC1 = 0x0011
    BC2 = 0x0021
    BC3 = 0x0031
    BC4 = 0x0041
    BC4_S = 0x1041
    BC5 = 0x0051
    BC5_S = 0x1051
    BC6H = 0x0061
    BC6H_SF = 0x1061
    BC7 = 0x0071
    ATI1N = 0x0141
    ATI2N = 0x0151
    ATI2N_XY = 0x0152
    ATI2N_DXT5 = 0x0153
    DXT1 = 0x0211
    DXT3 = 0x0221
    DXT5 = 0x0231
    DXT5_xGBR = 0x0252
    DXT5_RxBG = 0x0253
    DXT5_RBxG = 0x0254
    DXT5_xRBG = 0x0255
    DXT5_RGxB = 0x0256
    DXT5_xGxR = 0x0257
    ATC_RGB = 0x0301
    ATC_RGBA_Explicit = 0x0302
    ATC_RGBA_Interpolated = 0x0303
    ASTC = 0x0A01
    APC = 0x0A02
    PVRTC = 0x0A03
    ETC_RGB = 0x0E01
    ETC2_RGB = 0x0E02
    ETC2_SRGB = 0x0E03
    ETC2_RGBA = 0x0E04
    ETC2_RGBA1 = 0x0E05
    ETC2_SRGBA = 0x0E06
    ETC2_SRGBA1 = 0x0E07

    # New Compression Formats
    # -------------------------------------------------------------
    BINARY = 0x0B01
    GTC = 0x0B02
    BASIS = 0x0B03
    MAX = 0xFFFF


class CMP_Error(IntEnum):
    """
    Compress error codes

    Attributes
    ----------
    OK
        Ok.
    ABORTED
        The conversion was aborted.
    ERR_INVALID_SOURCE_TEXTURE
        The source texture is invalid.
    ERR_INVALID_DEST_TEXTURE
        The destination texture is invalid.
    ERR_UNSUPPORTED_SOURCE_FORMAT
        The source format is not a supported format.
    ERR_UNSUPPORTED_DEST_FORMAT
        The destination format is not a supported format.
    ERR_UNSUPPORTED_GPU_ASTC_DECODE
        The GPU hardware is not supported.
    ERR_UNSUPPORTED_GPU_BASIS_DECODE
        The GPU hardware is not supported.
    ERR_SIZE_MISMATCH
        The source and destination texture sizes do not match.
    ERR_UNABLE_TO_INIT_CODEC
        Compressonator was unable to initialize the codec needed for conversion.
    ERR_UNABLE_TO_INIT_DECOMPRESSLIB
        GPU_Decode Lib was unable to initialize the codec needed for decompression.
    ERR_UNABLE_TO_INIT_COMPUTELIB
        Compute Lib was unable to initialize the codec needed for compression.
    ERR_DESTINATION
        Error in compressing destination texture.
    ERR_MEM_ALLOC_FOR_MIPSET
        Memory Error: allocating MIPSet compression level data buffer.
    ERR_UNKNOWN_DESTINATION_FORMAT
        The destination Codec Type is unknown! In SDK refer to GetCodecType().
    ERR_FAILED_HOST_SETUP
        Failed to setup Host for processing.
    ERR_PLUGIN_FILE_NOT_FOUND
        The required plugin library was not found.
    ERR_UNABLE_TO_LOAD_FILE
        The requested file was not loaded.
    ERR_UNABLE_TO_CREATE_ENCODER
        Request to create an encoder failed.
    ERR_UNABLE_TO_LOAD_ENCODER
        Unable to load an encode library.
    ERR_NOSHADER_CODE_DEFINED
        No shader code is available for the requested framework.
    ERR_GPU_DOESNOT_SUPPORT_COMPUTE
        The GPU device selected does not support compute.
    ERR_NOPERFSTATS
        No Performance Stats are available.
    ERR_GPU_DOESNOT_SUPPORT_EXT
        The GPU does not support the requested compression extension!
    ERR_GAMMA_OUTOFRANGE
        Gamma value set for processing is out of range.
    ERR_PLUGIN_SHAREDIO_NOT_SET
        The plugin C_PluginSetSharedIO call was not set and is required for this plugin to operate.
    ERR_UNABLE_TO_INIT_D3DX
        Unable to initialize DirectX SDK or get a specific DX API.
    FRAMEWORK_NOT_INITIALIZED
        InitFramework failed or not called.
    ERR_GENERIC
        An unknown error occurred.
    """

    OK = 0
    ABORTED = 1
    ERR_INVALID_SOURCE_TEXTURE = 2
    ERR_INVALID_DEST_TEXTURE = 3
    ERR_UNSUPPORTED_SOURCE_FORMAT = 4
    ERR_UNSUPPORTED_DEST_FORMAT = 5
    ERR_UNSUPPORTED_GPU_ASTC_DECODE = 6
    ERR_UNSUPPORTED_GPU_BASIS_DECODE = 7
    ERR_SIZE_MISMATCH = 8
    ERR_UNABLE_TO_INIT_CODEC = 9
    ERR_UNABLE_TO_INIT_DECOMPRESSLIB = 10
    ERR_UNABLE_TO_INIT_COMPUTELIB = 11
    ERR_DESTINATION = 12
    ERR_MEM_ALLOC_FOR_MIPSET = 13
    ERR_UNKNOWN_DESTINATION_FORMAT = 14
    ERR_FAILED_HOST_SETUP = 15
    ERR_PLUGIN_FILE_NOT_FOUND = 16
    ERR_UNABLE_TO_LOAD_FILE = 17
    ERR_UNABLE_TO_CREATE_ENCODER = 18
    ERR_UNABLE_TO_LOAD_ENCODER = 19
    ERR_NOSHADER_CODE_DEFINED = 20
    ERR_GPU_DOESNOT_SUPPORT_COMPUTE = 21
    ERR_NOPERFSTATS = 22
    ERR_GPU_DOESNOT_SUPPORT_EXT = 23
    ERR_GAMMA_OUTOFRANGE = 24
    ERR_PLUGIN_SHAREDIO_NOT_SET = 25
    ERR_UNABLE_TO_INIT_D3DX = 26
    FRAMEWORK_NOT_INITIALIZED = 27
    ERR_GENERIC = 28


class CMP_ComputeType(IntEnum):
    """
    An enum selecting the different GPU driver types.

    Attributes
    ----------
    UNKNOWN
    CPU
        Use CPU Only, encoders defined CPUEncode or Compressonator lib will be used
    HPC
        Use CPU High Performance Compute Encoders with SPMD support defined in CPUEncode)
    GPU_OCL
        Use GPU Kernel Encoders to compress textures using OpenCL Framework
    GPU_DXC
        Use GPU Kernel Encoders to compress textures using DirectX Compute Framework
    GPU_VLK
        Use GPU Kernel Encoders to compress textures using Vulkan Compute Framework
    GPU_HW
        Use GPU HW to encode textures , using gl extensions
    """

    UNKNOWN = 0
    CPU = 1
    HPC = 2
    GPU_OCL = 3
    GPU_DXC = 4
    GPU_VLK = 5
    GPU_HW = 6


class CMP_Speed(IntEnum):
    """
    An enum selecting the speed vs. quality trade-off.

    Attributes
    ----------
    Normal
        Highest quality mode
    Fast
        Slightly lower quality but much faster compression mode - DXTn & ATInN only
    SuperFast
        Slightly lower quality but much, much faster compression mode - DXTn & ATInN only
    """

    Normal = 0
    Fast = 1
    SuperFast = 2


class CMP_GPUDecode(IntEnum):
    """
    An enum selecting the different GPU driver types.

    Attributes
    ----------
    OPENGL
        Use OpenGL to decode Textures (default)
    DIRECTX
        Use DirectX to decode Textures
    VULKAN
        Use Vulkan to decode Textures
    INVALID
    """

    OPENGL = 0
    DIRECTX = 1
    VULKAN = 2
    INVALID = 3


class CMP_ChannelFormat(IntEnum):
    """
    The format of data in the channels of texture.

    Attributes
    ----------
    _8bit
        8-bit integer data
    Float16
        16-bit float data
    Float32
        32-bit float data
    Compressed
        Compressed data
    _16bit
        16-bit integer data
    _2101010
        10-bit integer data in the color channels & 2-bit integer data in the alpha channel
    _32bit
        32-bit integer data
    Float9995E
        32-bit partial precision float
    YUV_420
        YUV Chroma formats
    YUV_422
        YUV Chroma formats
    YUV_444
        YUV Chroma formats
    YUV_4444
        YUV Chroma formats
    _1010102
        10-bit integer data in the color channels & 2-bit integer data in the alpha channel
    """

    _8bit = 0
    Float16 = 1
    Float32 = 2
    Compressed = 3
    _16bit = 4
    _2101010 = 5
    _32bit = 6
    Float9995E = 7
    YUV_420 = 8
    YUV_422 = 9
    YUV_444 = 10
    YUV_4444 = 11
    _1010102 = 12


class CMP_TextureDataType(IntEnum):
    """
    The type of data the texture represents.
    Do not change the index values, they are used in saved files

    Attributes
    ----------
    XRGB
        An RGB texture padded to DWORD width.
    ARGB
        An ARGB texture.
    NORMAL_MAP
        A normal map.
    R
        A single component texture.
    RG
        A two component texture.
    YUV_SD
        An YUB Standard Definition texture.
    YUV_HD
        An YUB High Definition texture.
    RGB
        An RGB texture
    _8
        8  Bit untyped data
    _16
        16 Bit untyped data
    """

    XRGB = 0
    ARGB = 1
    NORMAL_MAP = 2
    R = 3
    RG = 4
    YUV_SD = 5
    YUV_HD = 6
    RGB = 7
    _8 = 8
    _16 = 9


class CMP_TextureType(IntEnum):
    """
    The type of the texture or Data.
    Do not change the index values, they are used in saved files

    Attributes
    ----------
    _2D
        A regular 2D texture. data stored linearly (rgba,rgba,...rgba)
    CubeMap
        A cubemap texture.
    VolumeTexture
        A volume texture.
    _2D_Block
        2D texture data stored as [Height][Width] blocks as individual channels using cmp_rgb_t or cmp_yuv_t
    _1D
        Untyped data stored linearly
    Unknown
        Unknown type of texture : No data is stored for this type
    """

    _2D = 0
    CubeMap = 1
    VolumeTexture = 2
    _2D_Block = 3
    _1D = 4
    Unknown = 5


__all__ = ("CMP_Format", "CMP_Error", "CMP_ComputeType")
