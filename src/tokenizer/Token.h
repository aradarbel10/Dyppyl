#pragma once

#include <type_traits>

#include <variant>
#include <functional>
#include <unordered_map>
#include <string>
#include <map>
#include <string_view>
#include <variant>

#include <iostream>
#include "magic_enum/magic_enum.hpp"
#include "frozen/map.h"
#include "../Logger.h"

namespace dpl {

	struct Terminal {
		using terminal_value_type = std::variant<std::monostate, std::string_view>;
		enum class Type { Identifier, Number, String, Symbol, Keyword, Unknown, EndOfFile };

		Type type = Type::Unknown;
		terminal_value_type terminal_value = std::monostate{};

		constexpr friend auto operator<=>(const Terminal& lhs, const Terminal& rhs) {
			if (lhs == rhs) return std::strong_ordering::equal;
			else return std::tie(lhs.type, lhs.terminal_value) <=> std::tie(rhs.type, rhs.terminal_value);
		}

		constexpr friend bool operator==(const Terminal& lhs, const Terminal& rhs) {
			if (lhs.type == Type::Unknown || rhs.type == Type::Unknown) return true;
			else return lhs.type == rhs.type && lhs.terminal_value == rhs.terminal_value;
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
		constexpr Terminal(Type t, terminal_value_type v) : type(t), terminal_value(v) { }
		
	};



	struct Token : public Terminal {

		using Type = Terminal::Type;
		using Terminal::type;

		using value_type = std::variant<std::monostate, std::string, long double>;

		value_type value;
		std::pair<unsigned int, unsigned int> pos;

		friend constexpr auto operator<=>(const Token& lhs, const Token& rhs) {
			if (lhs.type == Type::Unknown || rhs.type == Type::Unknown) return std::partial_ordering::equivalent;
			return std::tie(lhs.type, lhs.value, lhs.pos) <=> std::tie(rhs.type, rhs.value, rhs.pos);
		}

		friend constexpr bool operator==(const Token& lhs, const Token& rhs) {
			if (lhs.type == Type::Unknown || rhs.type == Type::Unknown) return true;
			if (lhs.type != rhs.type) return false;
			if (std::holds_alternative<std::monostate>(lhs.value) || std::holds_alternative<std::monostate>(rhs.value)) {
				return true;
			}
			return lhs.value == rhs.value;
		}

		friend std::ostream& operator<<(std::ostream& os, const Token& t) {
			os << t.stringify();
			return os;
		}

		constexpr std::string stringify() const {
			std::string type_name(magic_enum::enum_name(type));

			// #TASK : convert this to a switch
			if (type == Type::Symbol) {
				return "[" + std::string{ std::get<std::string_view>(terminal_value) } + ", " + type_name + "]"; // "[+, Symbol]"
			} else if (type == Type::Keyword) {
				return "[" + std::string{ std::get<std::string_view>(terminal_value) } + ", " + type_name + "]"; // "[bool, Keyword]"
			} else if (type == Type::Identifier) {
				return "[" + std::get<std::string>(value) + ", " + type_name + "]"; // "[myVar, Identifier]"
			} else {
				// #TASK : don't print strings that are too long / span over multiple lines
				if (const auto* str = std::get_if<std::string>(&value))
					return "[\"" + *str + "\", " + type_name + "]"; // "["heya", String]"
				else if (const auto* dbl = std::get_if<long double>(&value))
					return "[" + std::to_string(*dbl) + ", " + type_name + "]"; // "[3.14, Number]"
				else return "[" + type_name + "]"; // "[Unknown]" || "[EndOfFile]"
			}
		}

		Token() = default;
		Token(Terminal t) : Terminal(t) { value = std::monostate(); }
		Token(Type t) : Terminal(t) { value = std::monostate(); }
		Token(Type t, std::string_view v) : Terminal(t) {
			if (type == Type::Symbol || type == Type::Keyword) {
				value = std::monostate{};
				terminal_value = v;
			} else {
				value = std::string{ v };
				terminal_value = std::monostate{};
			}
		}
		Token(Type t, long double num) : Terminal(t) {
			value = num;
		}
		//Token(Type t, std::string_view v) {
		//	type = t;
		//	if (type == Type::Symbol || type == Type::Keyword) terminal_value = v;
		//	else value = std::string{ v };
		//}
		//Token(Type t, terminal_value_type v) : Terminal(t, v) { }
	};

	namespace literals {
		constexpr dpl::Terminal operator"" _kwd(const char* str, size_t) {
			return { dpl::Terminal::Type::Keyword, std::string_view{str} };
		}

		constexpr dpl::Terminal operator"" _sym(const char* str, size_t) {
			return { dpl::Terminal::Type::Symbol, std::string_view{str} };
		}
	}
}

namespace std {
	template<> class hash<dpl::Token> {
	public:
		//credit to boost::hash_combine
		std::size_t operator()(dpl::Token const& t) const noexcept {
			size_t intermediate = std::hash<dpl::Token::Type>{}(t.type);
			intermediate ^= std::hash<typename dpl::Token::value_type>{}(t.value)
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