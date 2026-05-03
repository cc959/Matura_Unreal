#ifndef MSGPACK_PLATFORM_H
#define MSGPACK_PLATFORM_H

//*****************************************************************************
// Intrinsics, derived from the boost intrinsic
//*****************************************************************************

#ifndef __has_builtin         // Optional of course
#define __has_builtin(x) 0  // Compatibility with non-clang compilers
#endif

//  GCC and Clang recent versions provide intrinsic byte swaps via builtins
#if (defined(__clang__) && __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap64)) \
 || (defined(__GNUC__ ) && \
  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)))
# define PLATFORM_ENDIAN_INTRINSIC_MSG "__builtin_bswap16, etc."
// prior to 4.8, gcc did not provide __builtin_bswap16 on some platforms so we emulate it
// see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=52624
// Clang has a similar problem, but their feature test macros make it easier to detect
# if (defined(__clang__) && __has_builtin(__builtin_bswap16)) \
 || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))
#   define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_2(x) __builtin_bswap16(x)
# else
#   define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_2(x) __builtin_bswap32((x) << 16)
# endif
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_4(x) __builtin_bswap32(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_8(x) __builtin_bswap64(x)

//  Linux systems provide the byteswap.h header, with 
#elif defined(__linux__)
//  don't check for obsolete forms defined(linux) and defined(__linux) on the theory that
//  compilers that predefine only these are so old that byteswap.h probably isn't present.
# define PLATFORM_ENDIAN_INTRINSIC_MSG "byteswap.h bswap_16, etc."
# include <byteswap.h>
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_2(x) bswap_16(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_4(x) bswap_32(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_8(x) bswap_64(x)

#elif defined(_MSC_VER)
//  Microsoft documents these as being compatible since Windows 95 and specificly
//  lists runtime library support since Visual Studio 2003 (aka 7.1).
# define PLATFORM_ENDIAN_INTRINSIC_MSG "cstdlib _byteswap_ushort, etc."
# include <cstdlib>
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_2(x) _byteswap_ushort(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_4(x) _byteswap_ulong(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_8(x) _byteswap_uint64(x)
#else
# define PLATFORM_ENDIAN_NO_INTRINSICS
# define PLATFORM_ENDIAN_INTRINSIC_MSG "no byte swap intrinsics"
#endif

#if defined(__linux__)
#   include <endian.h>
#   if __BYTE_ORDER == __BIG_ENDIAN
#       define  PLATFORM_BIG_ENDIAN    1
#       define  PLATFORM_LITTLE_ENDIAN 0
#   else
#       define  PLATFORM_BIG_ENDIAN    0
#       define  PLATFORM_LITTLE_ENDIAN 1
#endif
#elif defined(__MACH__)
#   include <machine/endian.h>
#   if BYTE_ORDER == BIG_ENDIAN
#       define  PLATFORM_BIG_ENDIAN    1
#       define  PLATFORM_LITTLE_ENDIAN 0
#else
#       define  PLATFORM_BIG_ENDIAN    0
#       define  PLATFORM_LITTLE_ENDIAN 1
#endif
#elif defined(_WIN32)
#   define  PLATFORM_BIG_ENDIAN    0
#   define  PLATFORM_LITTLE_ENDIAN 1
#else
#error "Unable to determine OS endianness"
#endif

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <string>
#include <cstdio>

namespace platform {
template<typename T> constexpr typename std::enable_if<sizeof(T) == 1, T>::type byte_swap(const T t) {
    return t;
}
template<typename T> constexpr typename std::enable_if<sizeof(T) == 2, T>::type byte_swap(const T t) {
    return (T) (PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_2((uint16_t) t));
}
template<typename T> constexpr typename std::enable_if<sizeof(T) == 4, T>::type byte_swap(const T t) {
    return (T) (PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_4((uint32_t) t));
}
template<typename T> constexpr typename std::enable_if<sizeof(T) == 8, T>::type byte_swap(const T t) {
    return (T) (PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_8((uint64_t) t));
}
inline float byte_swap(const float t) {
    uint32_t tmp = PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_4(*((uint32_t*) &t));
    return *((float*) &tmp);
}
inline double byte_swap(const double t) {
    uint64_t tmp = PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_8(*((uint64_t*) &t));
    return *((double*) &tmp);
}

#if PLATFORM_BIG_ENDIAN
constexpr bool big_endian() { return true; }
constexpr bool little_endian() { return false; }
#else
constexpr bool big_endian() { return false; }
constexpr bool little_endian() { return true; }
#endif

template<typename T> constexpr T hton(T t) {
    return little_endian() ? byte_swap(t) : t;
}

template<typename T> constexpr T ntoh(T t) {
    return hton(t);
}

constexpr uint16_t hton_s(const uint16_t t) { return hton(t); }
constexpr uint32_t hton_l(const uint32_t t) { return hton(t); }
constexpr uint64_t hton_q(const uint64_t t) { return hton(t); }

constexpr uint16_t ntoh_s(const uint16_t t) { return ntoh(t); }
constexpr uint32_t ntoh_l(const uint32_t t) { return ntoh(t); }
constexpr uint64_t ntoh_q(const uint64_t t) { return ntoh(t); }

template<std::string::size_type N = 128, typename ... Args> std::string str_printf(const char* fmt, Args ...args) {
    std::string out;
    int size_used;

    for (std::string::size_type size_needed = N;; size_needed += N) {
        out.resize(size_needed);
        if ((size_used = snprintf(&out[0], size_needed, fmt, args...)) < 0) { return std::string("FMT ERROR"); }
        if (static_cast<std::string::size_type>(size_used) < size_needed) { break; }
    }
    out.resize(static_cast<std::string::size_type>(size_used));
    return out;
}
}

#endif //MSGPACK_PLATFORM_H
