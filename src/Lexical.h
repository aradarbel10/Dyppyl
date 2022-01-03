#pragma once

#include <vector>
#include <functional>
#include <any>

#include "hybrid/hybrid.hpp"

#include "tokenizer/Token.h"
#include "Regex.h"

namespace dpl {
	template<typename AtomT = char, typename TokenT = dpl::Token<>>
	class Lexeme {
		using atom_type = AtomT;
		using span_type = dpl::span_dict<atom_type>;

		using token_type = TokenT;

	public:
		std::function<token_type(typename token_type::name_type, span_type)> eval;
		dpl::regex_wrapper<atom_type> regex;

		template<typename Func>
			requires std::invocable<Func, span_type>
			&& std::constructible_from<token_type, typename token_type::name_type, std::invoke_result_t<Func, span_type>>
		constexpr Lexeme(dpl::regex auto r, Func f) : regex(r), eval([=](typename token_type::name_type name, span_type str) -> token_type {
			return token_type{ name, f(str) };
		}) {}

		constexpr Lexeme(dpl::regex auto r) : regex(r), eval([](typename token_type::name_type name, span_type str) -> token_type {
			if constexpr (std::constructible_from<token_type, typename token_type::name_type, span_type>) {
				return token_type{ name, str };
			} else return token_type{ typename token_type::terminal_type{ name } };
		}) {}

		constexpr Lexeme(dpl::regex auto r, typename token_type::name_type name)
		: regex(r), eval([=](typename token_type::name_type, span_type) -> token_type {
			return token_type{ typename token_type::terminal_type{ name } };
		}) {}
	};

	//template<typename AtomT = char, typename TokenT = dpl::Token<>>
	//struct Lexicon : public std::map<typename TokenT::name_type, dpl::Lexeme<AtomT, TokenT>> {
	//	using ::map;
	//};

	template<typename AtomT = char, typename TokenT = dpl::Token<>>
	using Lexicon = std::map<typename TokenT::name_type, dpl::Lexeme<AtomT, TokenT>>;
}