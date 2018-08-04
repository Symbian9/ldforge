#pragma once
#include <array>

namespace detail
{
	template<typename T, typename Vt>
	constexpr bool oneOfCompare(const T& value, Vt *first, Vt *last)
	{
		return (first == last)
			? false
			: (*first) == value
				? true
				: detail::oneOfCompare(value, first + 1, last);
	}

	template<typename Vt, std::size_t N>
	struct OneOfClass
	{
		std::array<Vt, N> parameters;

		template<typename... Ts>
		constexpr OneOfClass(Ts&&... parameters) :
			parameters {{parameters...}} {}

		template<typename T>
		constexpr bool operator==(const T& value) const
		{
			return detail::oneOfCompare(value, &parameters[0], &parameters[N]);
		}

		template<typename T>
		constexpr bool operator!=(const T& value) const
		{
			return !(*this == value);
		}

		template<typename T>
		constexpr friend bool operator==(const T& value, const OneOfClass<Vt, N>& array)
		{
			return array == value;
		}

		template<typename T>
		constexpr friend bool operator!=(const T& value, const OneOfClass<Vt, N>& array)
		{
			return array != value;
		}
	};
};

/*
 * Returns such an object that compares equal to some value if and only if any of the parameters do.
 * Example: (x == any(1, 2, 3)) is equivalent to (x == 1 || x == 2 || x == 3) other than the lack of lazy-evaluation
 */
template<typename... Ts>
constexpr auto oneOf(Ts&&... parameters)
{
	using Vt = typename std::common_type<Ts...>::type;
	return detail::OneOfClass<Vt, sizeof...(Ts)>({parameters...});
}

static_assert(oneOf(1,2,3) == 3, "oneOf unit test");
static_assert(oneOf(1,2,3) != 5, "oneOf unit test");
static_assert(3 == oneOf(1,2,3), "oneOf unit test");
static_assert(5 != oneOf(1,2,3), "oneOf unit test");
