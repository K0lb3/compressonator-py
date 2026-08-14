#include "compressonator.h"
#include <map>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <cstdint>

namespace nb = nanobind;

// Helper macros/constructors for cleaner dtype construction
constexpr nb::dlpack::dtype make_dtype(nb::dlpack::dtype_code code, uint8_t bits)
{
    return nb::dlpack::dtype{static_cast<uint8_t>(code), bits, 1};
}

constexpr auto DT_U8  = make_dtype(nb::dlpack::dtype_code::UInt, 8);
constexpr auto DT_I8  = make_dtype(nb::dlpack::dtype_code::Int, 8);
constexpr auto DT_U16 = make_dtype(nb::dlpack::dtype_code::UInt, 16);
constexpr auto DT_U32 = make_dtype(nb::dlpack::dtype_code::UInt, 32);
constexpr auto DT_F16 = make_dtype(nb::dlpack::dtype_code::Float, 16);
constexpr auto DT_F32 = make_dtype(nb::dlpack::dtype_code::Float, 32);

struct ChannelFormatInfo
{
    nb::dlpack::dtype dtype = DT_U8; // Default to unsigned 8-bit integer
    uint8_t channels = 0;            // 0 indicates compressed, packed, or block formats
    bool packed = false;
};

const std::map<CMP_FORMAT, ChannelFormatInfo> CHANNEL_FORMAT_INFO = {
    // Channel Component formats --------------------------------------------------------------------------------
    // Byte Format 0x0nn0
    {CMP_FORMAT_RGBA_8888_S, {DT_I8, 4}},         // RGBA format with signed 8-bit fixed channels.
    {CMP_FORMAT_ARGB_8888_S, {DT_I8, 4}},         // ARGB format with signed 8-bit fixed channels.
    {CMP_FORMAT_ARGB_8888, {DT_U8, 4}},           // ARGB format with 8-bit fixed channels.
    {CMP_FORMAT_ABGR_8888, {DT_U8, 4}},           // ABGR format with 8-bit fixed channels.
    {CMP_FORMAT_RGBA_8888, {DT_U8, 4}},           // RGBA format with 8-bit fixed channels.
    {CMP_FORMAT_BGRA_8888, {DT_U8, 4}},           // BGRA format with 8-bit fixed channels.
    {CMP_FORMAT_RGB_888, {DT_U8, 3}},             // RGB format with 8-bit fixed channels.
    {CMP_FORMAT_RGB_888_S, {DT_I8, 3}},           // RGB format with 8-bit fixed channels.
    {CMP_FORMAT_BGR_888, {DT_U8, 3}},             // BGR format with 8-bit fixed channels.
    {CMP_FORMAT_RG_8_S, {DT_I8, 2}},              // Two component format with signed 8-bit fixed channels.
    {CMP_FORMAT_RG_8, {DT_U8, 2}},                // Two component format with 8-bit fixed channels.
    {CMP_FORMAT_R_8_S, {DT_I8, 1}},               // Single component format with signed 8-bit fixed channel.
    {CMP_FORMAT_R_8, {DT_U8, 1}},                 // Single component format with 8-bit fixed channel.
    {CMP_FORMAT_ARGB_2101010, {DT_U32, 0, true}}, // ARGB format with 10-bit fixed channels for color & a 2-bit fixed channel for alpha.
    {CMP_FORMAT_RGBA_1010102, {DT_U32, 0, true}}, // RGBA format with 10-bit fixed channels for color & a 2-bit fixed channel for alpha.
    {CMP_FORMAT_ARGB_16, {DT_U16, 4}},            // ARGB format with 16-bit fixed channels.
    {CMP_FORMAT_ABGR_16, {DT_U16, 4}},            // ABGR format with 16-bit fixed channels.
    {CMP_FORMAT_RGBA_16, {DT_U16, 4}},            // RGBA format with 16-bit fixed channels.
    {CMP_FORMAT_BGRA_16, {DT_U16, 4}},            // BGRA format with 16-bit fixed channels.
    {CMP_FORMAT_RG_16, {DT_U16, 2}},              // Two component format with 16-bit fixed channels.
    {CMP_FORMAT_R_16, {DT_U16, 1}},               // Single component format with 16-bit fixed channels.

    // Float Format 0x1nn0
    {CMP_FORMAT_RGBE_32F, {DT_U32, 0, true}}, // RGB format with 9-bit floating point each channel and shared 5 bit exponent
    {CMP_FORMAT_ARGB_16F, {DT_F16, 4}}, // ARGB format with 16-bit floating-point channels.
    {CMP_FORMAT_ABGR_16F, {DT_F16, 4}}, // ABGR format with 16-bit floating-point channels.
    {CMP_FORMAT_RGBA_16F, {DT_F16, 4}}, // RGBA format with 16-bit floating-point channels.
    {CMP_FORMAT_BGRA_16F, {DT_F16, 4}}, // BGRA format with 16-bit floating-point channels.
    {CMP_FORMAT_RG_16F, {DT_F16, 2}},   // Two component format with 16-bit floating-point channels.
    {CMP_FORMAT_R_16F, {DT_F16, 1}},    // Single component with 16-bit floating-point channels.
    {CMP_FORMAT_ARGB_32F, {DT_F32, 4}}, // ARGB format with 32-bit floating-point channels.
    {CMP_FORMAT_ABGR_32F, {DT_F32, 4}}, // ABGR format with 32-bit floating-point channels.
    {CMP_FORMAT_RGBA_32F, {DT_F32, 4}}, // RGBA format with 32-bit floating-point channels.
    {CMP_FORMAT_BGRA_32F, {DT_F32, 4}}, // BGRA format with 32-bit floating-point channels.
    {CMP_FORMAT_RGB_32F, {DT_F32, 3}},  // RGB format with 32-bit floating-point channels.
    {CMP_FORMAT_BGR_32F, {DT_F32, 3}},  // BGR format with 32-bit floating-point channels.
    {CMP_FORMAT_RG_32F, {DT_F32, 2}},   // Two component format with 32-bit floating-point channels.
    {CMP_FORMAT_R_32F, {DT_F32, 1}},    // Single component with 32-bit floating-point channels.
};

const ChannelFormatInfo &getChannelFormatInfo(CMP_FORMAT format)
{
    auto it = CHANNEL_FORMAT_INFO.find(format);
    if (it != CHANNEL_FORMAT_INFO.end())
    {
        return it->second;
    }
    else
    {
        static const ChannelFormatInfo unknown_format_info = {DT_U8, 0}; // Default to unsigned 8-bit integer with 0 channels
        return unknown_format_info;
    }
}