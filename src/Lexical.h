#pragma once

#include <vector>
#include <functional>
#include <any>

#include "tokenizer/Token.h"
#include "Regex.h"

namespace dpl {
	template<typename AtomT = char, typename TokenT = dpl::Token<>>
	class Lexeme {
		using atom_type = AtomT;
		using span_type = dpl::span_dict<atom_type>;

		using token_type = TokenT;

	public:
		std::function<token_type(size_t, span_type)> eval;
		dpl::regex_wrapper<atom_type> regex;

		template<typename Func>
			requires std::invocable<Func, span_type>
			&& std::constructible_from<token_type, size_t, std::invoke_result_t<Func, span_type>>
		constexpr Lexeme(dpl::regex auto r, Func f) : regex(r), eval([=](size_t id, span_type str) -> token_type {
			return token_type{ id, f(str) };
		}) {}

		constexpr Lexeme(dpl::regex auto r) : regex(r), eval([](size_t id, span_type str) -> token_type {
			if constexpr (std::constructible_from<token_type, size_t, span_type>) {
				return token_type{ id, str };
			} else return token_type{ id };
		}) {}
	};

	template<typename AtomT = char, typename TokenT = dpl::Token<>>
	class Lexicon : public std::vector<Lexeme<AtomT, TokenT>> {
	public:
		constexpr Lexicon(std::initializer_list<Lexeme<AtomT, TokenT>> il)
			: std::vector<Lexeme<AtomT, TokenT>>(il) {}

	};
}