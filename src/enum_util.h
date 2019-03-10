#pragma once
/// Copyright (c) 2018 Vassily Checkin. See included license file.
#include <type_traits>
#include <algorithm>

/** boiler plate code for using "class enum" as bitmask
 * Use `ENABLE_BITMAP_OPERATORS(EnumName);` in global scope to enable bitwise operations
 * on enum value
 *
 */
template <typename A> struct is_bitmap : public std::false_type {};

/** bit of synt sugar for expressions like
 *  id == ID1 || id == ID2 || id == ID3
 *  oneOf(id, {ID1, ID2, ID3});
 */
template <typename T>
/* @TODO `typename` -> `std::equality_comparable` when <concepts> hdr is available*/
bool oneOf(const T &a, const std::initializer_list<T> &l)
{
    return std::any_of(l.begin(), l.end(), [&](auto &&e) { return a == e; });
}

/** bool conversion for bitmap enum */
template <typename Enum>
constexpr typename std::enable_if<is_bitmap<Enum>::value, bool>::type
isSet(const Enum &a)
{
    using ul = typename std::underlying_type<Enum>::type;
    return static_cast<ul>(a) != 0;
}

/// enable bitwise operations for enum class type Enum. Must be in the global namespace
#define ENABLE_BITMAP_OPERATORS(Enum) \
    template <> struct is_bitmap<Enum> : public std::is_enum<Enum> {};

template <typename Enum>
constexpr typename std::enable_if<is_bitmap<Enum>::value, Enum>::type
operator|(const Enum &a, const Enum &b)
{
    using ul = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<ul>(a) | static_cast<ul>(b));
}

template <typename Enum>
constexpr typename std::enable_if<is_bitmap<Enum>::value, Enum>::type
operator|=(Enum &a, const Enum &b)
{
    using ul = typename std::underlying_type<Enum>::type;
    return a = static_cast<Enum>(static_cast<ul>(a) | static_cast<ul>(b));
}

template <typename Enum>
constexpr typename std::enable_if<is_bitmap<Enum>::value, Enum>::type
operator&(const Enum &a, const Enum &b)
{
    using ul = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<ul>(a) & static_cast<ul>(b));
}

template <typename Enum>
constexpr typename std::enable_if<is_bitmap<Enum>::value, Enum>::type
operator&=(Enum &a, const Enum &b)
{
    using ul = typename std::underlying_type<Enum>::type;
    return a = static_cast<Enum>(static_cast<ul>(a) & static_cast<ul>(b));
}

template <typename Enum>
typename std::enable_if<is_bitmap<Enum>::value, Enum>::type
operator~(const Enum &a)
{
    using ul = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(~static_cast<ul>(a));
}
