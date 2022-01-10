#pragma once

#include <vector>
#include <functional>
#include <any>

#include "tokenizer/Token.h"
#include "Regex.h"

namespace dpl {
	struct discard_type {};
	static constexpr discard_type discard;


	template<typename AtomT = char, typename TokenT = dpl::Token<>>
	class Lexeme {
	public:
		using atom_type = AtomT;
		using span_type = dpl::span_dict<atom_type>;

		using token_type = TokenT;
		using regex_type = dpl::regex_wrapper<atom_type>;

	public:
		std::function<token_type(typename token_type::name_type, span_type)> eval;
		regex_type regex;

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
	};

	template<typename AtomT = char, typename TokenT = dpl::Token<>>
	struct Lexicon {
	public:
		using atom_type = AtomT;
		using token_type = TokenT;

		using name_type = token_type::name_type;
		using lexeme_type = dpl::Lexeme<atom_type, token_type>;
		using regex_type = lexeme_type::regex_type;
		using span_type = lexeme_type::span_type;

	private:
		std::unordered_map<name_type, std::pair<lexeme_type, short>> lexemes;

	public:
	
		std::optional<regex_type> discard_regex;

		auto begin() const { return lexemes.begin(); }
		auto end() const { return lexemes.end(); }

		constexpr const lexeme_type& at(const name_type& key) const { return lexemes.at(key).first; }
		constexpr bool contains(name_type name) const { return lexemes.contains(name); }
		constexpr void insert(std::pair<name_type, lexeme_type> pair) { lexemes.insert({ pair.first, {pair.second, static_cast<short>(lexemes.size()) } }); }
	};
}