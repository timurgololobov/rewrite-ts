#pragma once
#include <cstddef>      // std::size_t
#include <iomanip>      // std::quoted
#include <string>       // std::string
#include <tuple>        // std::tuple
#include <type_traits>  // std::declval, std::void_t,
                        // std::false_type, std::true_type
#include <utility>      // std::pair, std::tuple
                        // std::forward

#if __has_include(<variant>)
#include <variant>  // std::variant
#endif
#if __has_include(<optional>)
#include <optional>  // std::optional
#endif

namespace pretty::detail {

    template <typename T, typename = void>
    struct is_iterable : std::false_type {};
    template <typename T>
    struct is_iterable<T, std::void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>>
        : std::true_type {};

    template <typename T>
    inline constexpr bool is_iterable_v = is_iterable<T>::value || std::is_array_v<T>;

    template <typename Stream, typename, typename = void>
    struct has_ostream_operator : std::false_type {};
    template <typename Stream, typename T>
    struct has_ostream_operator<Stream, T, std::void_t<decltype(std::declval<Stream&>() << std::declval<const T&>())>>
        : std::true_type {};

    template <typename Stream, typename T>
    inline constexpr bool has_ostream_operator_v = has_ostream_operator<Stream, T>::value;


	template <typename T, typename T1, typename... Args>
	struct is_same_any_of {
		constexpr static bool test() {
			return (std::is_same_v<T, T1> || (std::is_same_v<T, Args> || ...));
		}
		static constexpr bool value = test();
	};

    //template <typename T, typename T1, typename... Args>
    //struct is_same_any_of {
    //    static constexpr bool value = std::is_same_v<T, T1> || (std::is_same_v<T, Args> || ...);
    //};
    template <typename T, typename T1, typename... Args>
    inline constexpr bool is_same_any_of_v = is_same_any_of<T, T1, Args...>::value;


    template <typename T>
    inline constexpr bool is_char_type_v =
        is_same_any_of_v<T, unsigned char, signed char, char, char16_t, char32_t, wchar_t>;

    template <typename T, typename = void>
    struct is_c_string : std::false_type {};
    template <typename T>
    struct is_c_string<T[]> : std::conditional_t<is_char_type_v<T>, std::true_type, std::false_type> {};
    template <typename T, size_t N>
    struct is_c_string<T[N]> : std::conditional_t<is_char_type_v<T>, std::true_type, std::false_type> {};

    template <typename T>
    inline constexpr bool is_c_string_v = is_c_string<T>::value;

    template <typename T, typename = void>
    struct is_map : std::false_type {};
    template <typename T>
    struct is_map<T, std::void_t<typename T::key_type, typename T::mapped_type,
                                 decltype(std::declval<T>()[std::declval<const typename T::key_type>()])>>
        : std::true_type {};
    template <typename T>
    inline constexpr bool is_map_v = is_map<T>::value;

    template <typename T, std::size_t N, typename = std::enable_if_t<is_char_type_v<T>>>
    auto quoted_helper(const T (&s)[N]) noexcept {
        return std::quoted(s);
    }

    inline auto quoted_helper(const char* c) noexcept { return std::quoted(c); }

    inline auto quoted_helper(const std::string& s) noexcept { return std::quoted(s); }

    inline auto quoted_helper(std::string& s) noexcept { return std::quoted(s); }

    template <class CharT, class Traits>
    auto quoted_helper(std::basic_string_view<CharT, Traits> s) noexcept {
        return std::quoted(s);
    }

    template <class T>
    decltype(auto) quoted_helper(T&& v) noexcept {
        return std::forward<T>(v);
    }

    struct ostream {  // struct ostream
        template <std::size_t Nested, class Stream, class T>
        static Stream& ostream_impl(Stream& out, const T& data);
        template <std::size_t Nested, class Stream, typename T, typename V,
                  typename = std::enable_if_t<!detail::has_ostream_operator_v<Stream, std::pair<T, V>>>>
        static Stream& ostream_impl(Stream& out, const std::pair<T, V>& data);
        template <std::size_t Nested, class Stream, typename... Args,
                  typename = std::enable_if_t<!detail::has_ostream_operator_v<Stream, std::tuple<Args...>>>>
        static Stream& ostream_impl(Stream& out, const std::tuple<Args...>& data);
#if __has_include(<optional>)
        template <std::size_t Nested, class Stream, typename T,
                  typename = std::enable_if_t<!detail::has_ostream_operator_v<Stream, std::optional<T>>>>
        static Stream& ostream_impl(Stream& out, const std::optional<T>& data);
#endif
#if __has_include(<variant>)
        template <std::size_t Nested, class Stream, typename T, typename... Ts>
        static Stream& ostream_impl(Stream& out, const std::variant<T, Ts...>& data);
#endif
    };  // struct ostream

    template <typename Stream, typename T>
    void append(Stream& out, T&& data) {
        out << std::forward<T>(data);
    }

    template <std::size_t Nested, class Stream, class Tuple, std::size_t... Is>
    void print_tuple_impl(Stream& out, const Tuple& value, std::index_sequence<Is...>) {
        ((void)(append(out, (Is == 0 ? "" : ", ")), (void)ostream::ostream_impl<Nested>(out, std::get<Is>(value))),
         ...);
    }

    template <std::size_t Nested, class Stream, class T>
    Stream& ostream::ostream_impl(Stream& out, const T& data) {
        if constexpr (detail::is_iterable_v<T> && !detail::is_c_string_v<T> &&
                      ((!detail::has_ostream_operator_v<Stream, T>) || std::is_array_v<T>)) {
            std::string delimiter;
            if constexpr (is_map_v<T>) {
                append(out, '{');
            } else {
                append(out, '[');
            }

            for (const auto& el : data) {
                append(out, delimiter);
                ostream_impl<Nested + 1>(out, detail::quoted_helper(el));
                delimiter = ", ";
            }

            if constexpr (is_map_v<T>) {
                append(out, '}');
            } else {
                append(out, ']');
            }
        } else if constexpr (detail::has_ostream_operator_v<Stream, T>) {
            append(out, detail::quoted_helper(data));
        } else if constexpr (std::is_enum_v<T>) {
            append(out, static_cast<std::underlying_type_t<T>>(data));
        } else {
            static_assert(detail::has_ostream_operator_v<Stream, T> && !std::is_enum_v<T>,
                          "not support [ostream& operator<<(ostream& out, const T& data)]");
        }

        return out;
    }

    template <std::size_t Nested, class Stream, typename T, typename V, typename>
    Stream& ostream::ostream_impl(Stream& out, const std::pair<T, V>& data) {
        if constexpr (detail::has_ostream_operator_v<Stream, std::pair<T, V>>) {
            append(out, data);
        } else {
            ///*if (!!Nested) */ out << '{';
            ostream_impl<Nested + 1>(out, detail::quoted_helper(data.first));
            append(out, ": ");
            ostream_impl<Nested + 1>(out, detail::quoted_helper(data.second));
            ///*if (!!Nested)*/ out << '}';
        }
        return out;
    }

    template <std::size_t Nested, class Stream, typename... Args, typename>
    Stream& ostream::ostream_impl(Stream& out, const std::tuple<Args...>& data) {
        append(out, "(");
        detail::print_tuple_impl<Nested>(out, data, std::index_sequence_for<Args...>{});
        append(out, ")");
        return out;
    }

#if __has_include(<optional>)
    template <std::size_t Nested, class Stream, typename T, typename>
    Stream& ostream::ostream_impl(Stream& out, const std::optional<T>& data) {
        if (data) {
            ostream_impl<Nested>(out, detail::quoted_helper(data.value()));
        } else {
            append(out, "null");
        }
        return out;
    }
#endif

#if __has_include(<variant>)
    template <std::size_t Nested, class Stream, typename T, typename... Ts>
    Stream& ostream::ostream_impl(Stream& out, const std::variant<T, Ts...>& data) {
        if (data.index() != std::variant_npos) {
            std::visit([&out](const auto& t) { ostream_impl<Nested>(out, t); }, data);
            return out;
        }
        append(out, "VARIANT_NPOS");
        return out;
    }
#endif


    /// static assert test
    static_assert(is_same_any_of_v<char, int, char, double>, "test failed");

#if __has_include(<variant>)
    static_assert(!is_iterable_v<std::variant<int, int>>, "test failed");
#endif
    static_assert(is_iterable_v<std::string>, "test failed");

    static_assert(!has_ostream_operator_v<std::ostream, std::pair<int, int>>, "test failed");
    static_assert(has_ostream_operator_v<std::ostream, std::string>, "test failed");

    static_assert(is_char_type_v<char>, "test failed");
    static_assert(is_char_type_v<signed char>, "test failed");
    static_assert(is_char_type_v<char>, "test failed");
    static_assert(is_char_type_v<char16_t>, "test failed");
    static_assert(is_char_type_v<char>, "test failed");
    static_assert(is_char_type_v<char32_t>, "test failed");
    static_assert(is_char_type_v<wchar_t>, "test failed");
    static_assert(!is_char_type_v<uint16_t>, "test failed");
    static_assert(!is_char_type_v<uint32_t>, "test failed");

}  // namespace pretty::detail
