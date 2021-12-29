#pragma once

#include <vector>
#include <functional>
#include <any>

#include "tokenizer/Token.h"
#include "Regex.h"

namespace dpl {
	using terminal_id = size_t;

	struct Token_ {
		constexpr Token_(auto&& v) : value(v) {}

		terminal_id id = -1;
		std::any value;
	};

	template<typename AtomT = char, typename TokenT = dpl::Token<>>
	class Lexeme {
		using atom_type = AtomT;
		using span_type = dpl::span_dict<atom_type>;

		using token_type = TokenT;

	public:
		std::function<token_type(span_type)> eval;
		dpl::regex_wrapper<atom_type> regex;

		template<typename Func>
			//requires std::invocable<Func, dpl::span_dict<AtomT>>
			//&& std::is_same_v<dpl::Token, std::invoke_result_t<Func, span_type>>
		constexpr Lexeme(dpl::regex auto r, Func f) : regex(r), eval(f) {}
		constexpr Lexeme(dpl::regex auto r) : regex(r), eval([](span_type str) -> token_type { return {} }) {}
	};

	template<typename AtomT = char>
	class Lexicon : public std::vector<Lexeme<AtomT>> {
	public:
		constexpr Lexicon(std::initializer_list<Lexeme<AtomT>> il) : std::vector<Lexeme<AtomT>>(il) {}

	};
}