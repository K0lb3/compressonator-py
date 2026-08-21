#pragma once

#include <cstddef>

// Define a smart NULL wrapper that converts to both bool and pointers
struct CMP_NullFix
{
    // Allows (bool == NULL) -> converts CMP_NullFix to bool (false)
    constexpr operator bool() const noexcept { return false; }

    // Allows (pointer == NULL) -> converts CMP_NullFix to any pointer type
    template <typename T>
    constexpr operator T *() const noexcept { return nullptr; }

    // Explicit operator== overload for bool comparisons
    constexpr bool operator==(bool b) const noexcept { return !b; }
    constexpr bool operator!=(bool b) const noexcept { return b; }
};

// Mirror operator== for symmetrical checks: (bool == CMP_NullFix)
inline constexpr bool operator==(bool b, CMP_NullFix n) noexcept
{
    return n == b;
}

inline constexpr bool operator!=(bool b, CMP_NullFix n) noexcept
{
    return n != b;
}

// Force standard libraries to pull in their headers first
#include <cstdlib>

// Undefine NULL and replace it with our type
#undef NULL
#define NULL \
    CMP_NullFix {}
