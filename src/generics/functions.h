#pragma once
#include <functional>
#include <cmath>
#include "../basics.h"

using std::abs;
using std::acos;
using std::asin;
using std::atan;
using std::atan2;
using std::ceil;
using std::cos;
using std::floor;
using std::function;
using std::hypot;
using std::log;
using std::log2;
using std::log10;
using std::pow;
using std::sin;
using std::sort;
using std::sqrt;

/*
 * Returns whether the argument is reasonably close to zero.
 */
template<typename T>
constexpr bool isZero(T a)
{
	return qFuzzyCompare(a + 1.0, 1.0);
}

template<typename T>
bool isInteger(T a)
{
	return (::abs(a - ::floor(a)) < 0.00001) or (::abs(a - ::ceil(a)) < 0.00001);
}

template<typename T>
auto constexpr squared(T value)
{
	return value * value;
}

//
// Returns true if first arg is equal to any of the other args
//
template<typename T, typename Arg, typename... Args>
bool isOneOf(const T& needle, const Arg& arg, const Args&... args)
{
	if (needle == arg)
		return true;
	else
		return isOneOf(needle, args...);
}

template<typename T>
bool isOneOf(const T&)
{
	return false;
}

// http://stackoverflow.com/a/18204188/3629665
template<typename T>
constexpr int rotl10(T x)
{
	return (x << 10) | ((x >> 22) & 0x000000ff);
}

template<typename T>
constexpr int rotl20(T x)
{
	return (x << 20) | ((x >> 12) & 0x000000ff);
}

//
// Get the amount of elements in something.
//
template<typename T, int N>
constexpr int countof(T(&)[N])
{
	return N;
}

[[maybe_unused]]
static inline int countof(const QString& string)
{
	return string.length();
}

template<typename T>
int countof(const QVector<T>& vector)
{
	return vector.size();
}

template<typename T>
int countof(const QList<T>& list)
{
	return list.size();
}

template<typename T>
int countof(const QSet<T>& set)
{
	return set.size();
}

template<typename T>
int countof(const std::initializer_list<T>& vector)
{
	return vector.size();
}

/*
 * Extracts the sign of 'value'.
 * From: https://stackoverflow.com/q/1903954
 */
template<typename T>
constexpr int sign(T value)
{
	return (0 < value) - (value < 0);
}

/*
 * Returns the maximum of a single parameter (the parameter itself).
 */
template <typename T>
constexpr T max(T a)
{
	return a;
}

/*
 * Returns the maximum of two parameters.
 */
template <typename T>
constexpr T max(T a, T b)
{
	return a > b ? a : b;
}

/*
 * Returns the maximum of n parameters.
 */
template <typename T, typename... Rest>
constexpr T max(T a, Rest&&... rest)
{
	return max(a, max(rest...));
}

/*
 * Returns the minimum of a single parameter (the parameter itself).
 */
template <typename T>
constexpr T min(T a)
{
	return a;
}

/*
 * Returns the minimum of two parameters.
 */
template <typename T>
constexpr T min(T a, T b)
{
	return a < b ? a : b;
}

/*
 * Returns the minimum of n parameters.
 */
template <typename T, typename... Rest>
constexpr T min(T a, Rest&&... rest)
{
	return min(a, min(rest...));
}

/*
 * Assigns the value of a single flag in a flagset
 */
template<int Flag, typename T>
void assignFlag(QFlags<T>& flagset, bool value)
{
	if (value)
		flagset |= static_cast<T>(Flag);
	else
		flagset &= ~static_cast<T>(Flag);
}

/*
 * Returns a singleton of type T, useful for providing a valid but unused
 * pointer.
 */
template<typename T>
inline T& singleton()
{
	static T result;
	return result;
}

/*
 * Rounds the input value to the nearest multiple of the provided interval.
 */
template<typename T>
T roundToInterval(T value, double interval)
{
	return static_cast<T>(round(value / interval) * interval);
}

/*
 * Returns the empty sum. (recursion base)
 */
template<typename T>
constexpr T sum()
{
	return {};
}

/*
 * Returns the sum of n arguments.
 */
template<typename T, typename... Rest>
constexpr T sum(const T& arg, Rest&&... rest)
{
	return arg + sum<T>(rest...);
}

// Copy of qOverload so as to drop Qt version requirement from 5.7 to 5.5.
#if (QT_VERSION < QT_VERSION_CHECK(5, 7, 0))
template <typename... Args>
struct QNonConstOverload
{
	template <typename R, typename T>
	Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
	template <typename R, typename T>
	static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};
template <typename... Args>
struct QConstOverload
{
	template <typename R, typename T>
	Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...) const) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
	template <typename R, typename T>
	static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...) const) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};
template <typename... Args>
struct QOverload : QConstOverload<Args...>, QNonConstOverload<Args...>
{
	using QConstOverload<Args...>::of;
	using QConstOverload<Args...>::operator();
	using QNonConstOverload<Args...>::of;
	using QNonConstOverload<Args...>::operator();
	template <typename R>
	Q_DECL_CONSTEXPR auto operator()(R (*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
	template <typename R>
	static Q_DECL_CONSTEXPR auto of(R (*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};
template <typename... Args> Q_CONSTEXPR Q_DECL_UNUSED QOverload<Args...> qOverload = {};
template <typename... Args> Q_CONSTEXPR Q_DECL_UNUSED QConstOverload<Args...> qConstOverload = {};
template <typename... Args> Q_CONSTEXPR Q_DECL_UNUSED QNonConstOverload<Args...> qNonConstOverload = {};
#endif
