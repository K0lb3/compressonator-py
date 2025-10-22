#include <type_traits>
#include "structmember.h"

// Primary template (undefined to force specialization)
template <typename T>
constexpr int GetPythonTypeCode();

// Specializations
template <>
constexpr int GetPythonTypeCode<uint8_t>() { return T_UBYTE; }
template <>
constexpr int GetPythonTypeCode<int8_t>() { return T_BYTE; }
template <>
constexpr int GetPythonTypeCode<uint16_t>() { return T_USHORT; }
template <>
constexpr int GetPythonTypeCode<int16_t>() { return T_SHORT; }
template <>
constexpr int GetPythonTypeCode<uint32_t>() { return T_UINT; }
template <>
constexpr int GetPythonTypeCode<int32_t>() { return T_INT; }
template <>
constexpr int GetPythonTypeCode<uint64_t>() { return T_ULONGLONG; }
template <>
constexpr int GetPythonTypeCode<int64_t>() { return T_LONGLONG; }
template <>
constexpr int GetPythonTypeCode<float>() { return T_FLOAT; }
template <>
constexpr int GetPythonTypeCode<double>() { return T_DOUBLE; }

// Helper for enums
template <typename Enum>
constexpr int GetEnumPythonTypeCode()
{
    return GetPythonTypeCode<std::underlying_type_t<Enum>>();
}