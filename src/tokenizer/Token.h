#pragma once

#include <type_traits>

#include <variant>
#include <functional>
#include <unordered_map>
#include <string>
#include <map>
#include <string_view>
#include <variant>
#include <any>

#include <iostream>
#include "magic_enum/magic_enum.hpp"
#include "../Logger.h"
#include "../Regex.h"

namespace dpl {

	struct Terminal {
		using terminal_id_type = size_t;

		terminal_id_type id = -1;
		bool is_wildcard = false;

		constexpr friend auto operator<=>(const Terminal& lhs, const Terminal& rhs) {
			if (lhs == rhs) return std::strong_ordering::equal;
			else return std::tie(lhs.id) <=> std::tie(rhs.id);
		}

		constexpr friend bool operator==(const Terminal& lhs, const Terminal& rhs) {
			if (lhs.is_wildcard || rhs.is_wildcard) return true;
			else return lhs.id == rhs.id;
		}

		friend std::ostream& operator<<(std::ostream& os, const Terminal& t) {
			os << t.stringify();
			return os;
		}

		constexpr std::string stringify() const {
			std::string type_name{ magic_enum::enum_name(type) };
			if (std::holds_alternative<std::monostate>(terminal_value)) return type_name;

			switch (type) {
			case Type::Symbol:
				return std::string{ std::get<std::string_view>(terminal_value) };
				break;
			case Type::Keyword:
				return std::string{ std::get<std::string_view>(terminal_value) };
				break;
			default:
				return type_name;
				break;
			}
		}

		constexpr Terminal() = default;
		constexpr Terminal(Type t) : type(t) { }
		constexpr Terminal(terminal_value_type v) : terminal_value(v) { }
		constexpr Terminal(Type t, terminal_value_type v) : type(t), terminal_value(v) { }

	};

	template<typename T = std::variant<std::monostate, std::string, long double>>
	struct Token : public Terminal {

		using Type = Terminal::Type;
		using Terminal::type;

		using value_type = T;

		value_type value;
		file_pos_t pos;

		friend constexpr auto operator<=>(const Token& lhs, const Token& rhs) {
			if (lhs.is_wildcard || rhs.is_wildcard) return std::partial_ordering::equivalent;
			return std::tie(lhs.type, lhs.value, lhs.pos) <=> std::tie(rhs.type, rhs.value, rhs.pos);
		}

		friend constexpr bool operator==(const Token& lhs, const Token& rhs) {
			if (lhs.is_wildcard || rhs.is_wildcard) return true;
			if (lhs.type != rhs.type) return false;
			if (lhs.terminal_value != rhs.terminal_value) return false;
			return lhs.value == rhs.value;
		}

		friend std::ostream& operator<<(std::ostream& os, const Token& t) {
			os << t.stringify();
			return os;
		}

		constexpr std::string stringify() const {
			if (is_wildcard) return std::string{ "[wildcard]" };
			else {
				std::ostringstream str;
				str << '[' << dpl::streamer{ type } << ", " << dpl::streamer{ value } << ']';
				return std::string{ str.str() };
			}
		}

		Token() = default;
		Token(Terminal t) : Terminal(t) { value = std::monostate(); }
		Token(Type t) : Terminal(t) { value = std::monostate(); }
		Token(Type t, value_type v) : Terminal(t), value(v) {}
	};

	namespace literals {
		constexpr dpl::Terminal operator"" _kwd(const char* str, size_t) {
			return { dpl::Terminal::Type::Keyword, str };
		}

		constexpr dpl::Terminal operator"" _sym(const char* str, size_t) {
			return { dpl::Terminal::Type::Symbol, str };
		}
	}
}

namespace std {
	template<typename T>
		requires requires { std::hash<T>{}; }
	class hash<dpl::Token<T>> {
	public:
		//credit to boost::hash_combine
		std::size_t operator()(dpl::Token<T> const& t) const noexcept {
			size_t intermediate = std::hash<dpl::Token<T>::Type>{}(t.type);
			intermediate ^= std::hash<typename dpl::Token<T>::value_type>{}(t.value)
				+ 0x9e3779b9 + (intermediate << 6) + (intermediate >> 2);
			return intermediate;
		}
	};

	template<> class hash<dpl::Terminal> {
	public:
		//credit to boost::hash_combine
		std::size_t operator()(dpl::Terminal const& t) const noexcept {
			size_t intermediate = std::hash<dpl::Terminal::Type>{}(t.type);
			intermediate ^= std::hash<typename dpl::Terminal::terminal_value_type>{}(t.terminal_value)
				+ 0x9e3779b9 + (intermediate << 6) + (intermediate >> 2);
			return intermediate;
		}
	};
}