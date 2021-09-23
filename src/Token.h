#pragma once

#include <type_traits>

#include <variant>
#include <functional>

#ifdef DPL_LOG
#include <iostream>
#include "magic_enum/magic_enum.hpp"
#include "Logger.h"
#endif //DPL_LOG

namespace dpl {

	enum class TokenType { Identifier, Number, String, Symbol, Keyword, Whitespace, Unknown, EndOfFile };

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	struct Token {
		using Type = TokenType;
		using value_type = std::variant<std::monostate, std::string, long double, SymT, KwdT>;

		Type type;
		value_type value;

		std::pair<unsigned int, unsigned int> pos;

		constexpr std::string stringify() const {
			if (type == TokenType::Symbol || type == TokenType::Keyword) {
				if (const auto* sym = std::get_if<SymT>(&value)) return std::string{ magic_enum::enum_name(*sym) };
				else if (const auto* kwd = std::get_if<KwdT>(&value)) return std::string{ magic_enum::enum_name(*kwd) };
				return "unknown enum";
			} else if (type == TokenType::Identifier) {
				return std::get<std::string>(value);
			} else if (type == TokenType::EndOfFile) {
				return "EOF";
			} else {
				std::string type_name(magic_enum::enum_name(type));
				if (const auto* str = std::get_if<std::string>(&value)) return type_name + " " + *str;
				else if (const auto* dbl = std::get_if<long double>(&value)) return type_name + " " + std::to_string(*dbl);
				return "unknown non-enum";
			}
		}

		constexpr Token() = default;
		constexpr Token(Type t, value_type v) : type(t), value(v) { }
		constexpr Token(Type t) : type(t), value(std::monostate()) { }
		constexpr Token(KwdT t) : type(Type::Keyword), value(t) { }
		constexpr Token(SymT t) : type(Type::Symbol), value(t) { }
	};

	template<typename KwdT, typename SymT>
	std::ostream& operator<<(std::ostream& os, const Token<KwdT, SymT>& t) {
		os << "[" << magic_enum::enum_name(t.type) << ", ";

		std::cout << dpl::log::streamer{ t.value };

		os << "]";
		return os;
	}

	template<typename KwdT, typename SymT>
	inline constexpr bool operator==(const Token<KwdT, SymT>& lhs, const Token<KwdT, SymT>& rhs) {
		if (lhs.type != rhs.type) return false;
		if (std::holds_alternative<std::monostate>(lhs.value) || std::holds_alternative<std::monostate>(rhs.value)) {
			return true;
		}
		return lhs.value == rhs.value;
	}

	template<typename KwdT, typename SymT>
	inline Token<KwdT, SymT> getTerminalType(const Token<KwdT, SymT>& tkn) {
		if (const auto* sym = std::get_if<SymT>(&tkn.value)){
			return *sym;
		} else if(const auto* kwd = std::get_if<KwdT>(&tkn.value)) {
			return *kwd;
		}
		return tkn.type;
	}
}

namespace std {
	template<typename KwdT, typename SymT> class hash<dpl::Token<KwdT, SymT>> {
	public:
		//credit to boost::hash_combine
		std::size_t operator()(dpl::Token<KwdT, SymT> const& t) const noexcept {
			size_t intermediate = std::hash<dpl::TokenType>{}(t.type);
			intermediate ^= std::hash<typename dpl::Token<KwdT, SymT>::value_type>{}(t.value)
				+ 0x9e3779b9 + (intermediate << 6) + (intermediate >> 2);
			return intermediate;
		}
	};
}