#pragma once

#include <cstddef>

struct CMP_NullFix
{
    // Allows implicit conversion to any pointer type
    template <typename T>
    constexpr operator T *() const noexcept { return nullptr; }

    // Allows comparison with bool: (CMP_NullFix == bool)
    constexpr operator bool() const noexcept { return false; }

    // EXPLICIT pointer comparison overloads to prevent compiler ambiguity
    template <typename T>
    constexpr bool operator==(T *ptr) const noexcept { return ptr == nullptr; }

    template <typename T>
    constexpr bool operator!=(T *ptr) const noexcept { return ptr != nullptr; }

    // Explicit bool comparisons
    constexpr bool operator==(bool b) const noexcept { return !b; }
    constexpr bool operator!=(bool b) const noexcept { return b; }
};

// Symmetric pointer comparisons: (PyObject* != CMP_NullFix)
template <typename T>
inline constexpr bool operator==(T *ptr, CMP_NullFix n) noexcept { return n == ptr; }

template <typename T>
inline constexpr bool operator!=(T *ptr, CMP_NullFix n) noexcept { return n != ptr; }

// Symmetric bool comparisons: (bool == CMP_NullFix)
inline constexpr bool operator==(bool b, CMP_NullFix n) noexcept { return n == b; }
inline constexpr bool operator!=(bool b, CMP_NullFix n) noexcept { return n != b; }

// Force standard headers to load their default macros first
#include <cstdlib>

// Undefine NULL and swap with our hybrid wrapper
#if defined(__musl__) || !defined(__GLIBC__)
#undef NULL
#define NULL \
    CMP_NullFix {}
#endif