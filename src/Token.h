#pragma once

#include <type_traits>

#include <variant>
#include <functional>

#include <iostream>

#include "magic_enum/magic_enum.hpp"

namespace dpl {

	enum class TokenType { Identifier, Number, String, Symbol, Keyword, Whitespace, Unknown, EndOfFile };

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	struct Token {
		using Type = TokenType;

		Type type;
		std::variant<std::monostate, std::string, double, SymT, KwdT> value;

		constexpr Token() = default;
		constexpr Token(Type t, auto v) : type(t), value(v) { }
		constexpr Token(Type t) : type(t), value(std::monostate()) { }
		constexpr Token(KwdT t) : type(Type::Keyword), value(t) { }
		constexpr Token(SymT t) : type(Type::Symbol), value(t) { }
	};

	template<typename KwdT, typename SymT>
	inline bool operator==(const Token<KwdT, SymT>& lhs, const Token<KwdT, SymT>& rhs) {
		return lhs.type == rhs.type && lhs.value == rhs.value;
	}

	template<typename KwdT, typename SymT>
	inline bool tokensCompareType(const Token<KwdT, SymT>& lhs, const Token<KwdT, SymT>& rhs) {
		if (lhs.type != rhs.type) return false;
		if (lhs.type == TokenType::Symbol || lhs.type == TokenType::Keyword) {
			return lhs.value == rhs.value;
		}
		return true;
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

	template<typename KwdT, typename SymT>
	std::ostream& operator<<(std::ostream& os, const Token<KwdT, SymT>& t) {
		const auto* sval = std::get_if<std::string>(&t.value);
		const auto* dval = std::get_if<double>(&t.value);
		const auto* smval = std::get_if<SymT>(&t.value);
		const auto* kwval = std::get_if<KwdT>(&t.value);

		os << "[" << magic_enum::enum_name(t.type) << ", ";

		if (sval) os << *sval;
		if (dval) os << *dval;
		if (smval) os << magic_enum::enum_name(*smval);
		if (kwval) os << magic_enum::enum_name(*kwval);

		os << "]";
		return os;
	}
}

namespace std {
	template<typename KwdT, typename SymT> class hash<dpl::Token<KwdT, SymT>> {
	public:
		//credit to boost::hash_combine
		std::size_t operator()(dpl::Token<KwdT, SymT> const& t) const noexcept {
			size_t intermediate = std::hash<dpl::TokenType>{}(t.type);
			intermediate ^= std::hash<std::variant<std::monostate, std::string, double, SymT, KwdT>>{}(t.value)
				+ 0x9e3779b9 + (intermediate << 6) + (intermediate >> 2);
			return intermediate;
		}
	};
}