#pragma once

#include <type_traits>

#include <variant>

#include <iostream>

#include "magic_enum/magic_enum.hpp"

namespace dpl {

	enum class TokenType { Identifier, Number, String, Symbol, Keyword, Whitespace, Unknown };

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