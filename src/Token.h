#pragma once

#include <type_traits>

#include <variant>
#include <functional>
#include <unordered_map>
#include <map>

#ifdef DPL_LOG
#include <iostream>
#include "magic_enum/magic_enum.hpp"
#include "Logger.h"
#endif //DPL_LOG

namespace dpl {

	struct Token {
		static std::map<std::string_view, size_t> keywords, symbols;

		enum class Type { Identifier, Number, String, Symbol, Keyword, Whitespace, Unknown, EndOfFile };
		using value_type = std::variant<std::monostate, std::string, long double, size_t>;

		Type type;
		value_type value;

		std::pair<unsigned int, unsigned int> pos;

		friend inline constexpr auto operator<=>(const Token& lhs, const Token& rhs) {
			return std::tie(lhs.type, lhs.value, lhs. pos) <=> std::tie(rhs.type, rhs.value, rhs.pos);
		}

		friend inline constexpr bool operator==(const Token& lhs, const Token& rhs) {
			if (lhs.type != rhs.type) return false;
			if (std::holds_alternative<std::monostate>(lhs.value) || std::holds_alternative<std::monostate>(rhs.value)) {
				return true;
			}
			return lhs.value == rhs.value;
		}

		static std::string_view symbolByIndex(size_t index) {
			for (const auto& [key, val] : symbols) {
				if (index == val) return key;
			}
			return "";
		}

		static std::string_view keywordByIndex(size_t index) {
			for (const auto& [key, val] : keywords) {
				if (index == val) return key;
			}
			return "";
		}

		constexpr std::string stringify() const {
			std::string type_name(magic_enum::enum_name(type));
			if (std::holds_alternative<std::monostate>(value)) return type_name;

			if (type == Type::Symbol) {
				return std::string{ symbolByIndex(std::get<size_t>(value)) };
			} else if (type == Type::Keyword) {
				return std::string{ keywordByIndex(std::get<size_t>(value)) };
			} else if (type == Type::Identifier) {
				return std::get<std::string>(value);
			} else if (type == Type::EndOfFile) {
				return "EOF";
			} else {
				
				if (const auto* str = std::get_if<std::string>(&value)) return type_name + " " + *str;
				else if (const auto* dbl = std::get_if<long double>(&value)) return type_name + " " + std::to_string(*dbl);
				else return type_name;
			}
		}

		Token() = default;
		Token(Type t) : type(t) {
			value = std::monostate();
			//if (type == Type::Identifier) value = "Identifier";
			//else if (type == Type::Number) value = "Number";
			//else if (type == Type::String) value = "String";
			//else 
		}
		Token(Type t, value_type v) : type(t), value(v) { }
	};

	std::map<std::string_view, size_t> Token::keywords;
	std::map<std::string_view, size_t> Token::symbols;

	std::ostream& operator<<(std::ostream& os, const Token& t) {
		os << "[" << magic_enum::enum_name(t.type) << ", ";

		std::cout << dpl::log::streamer{ t.value };

		os << "]";
		return os;
	}

	inline Token getTerminalType(const Token& tkn) {
		if (const auto* val = std::get_if<size_t>(&tkn.value)) return Token{ tkn.type, *val };
		else return tkn.type;
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
}