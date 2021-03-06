#pragma once

#include <string_view>
#include <vector>
#include <variant>
#include <utility>

namespace dpl {

	// forward declaration
	template<typename TokenT, typename NonterminalT, typename TerminalNameT>
	class Grammar;

	enum class TreeModifier;

	struct NonterminalLit : public std::string_view {
		TreeModifier modifier{ TreeModifier::None };
	};
	struct TerminalLit : public std::string_view {
		TreeModifier modifier{ TreeModifier::None };
	};

	using Prec = short;

	struct void_token_t {
		struct terminal_type {};
		struct name_type {};
	};

	struct ProdLit {
		std::vector<std::variant<NonterminalLit, TerminalLit>> sentence;
		dpl::Assoc assoc = dpl::Assoc::None;
		dpl::Prec prec = 0;
	};

	static const ProdLit epsilon{ .sentence = {} };

	struct NtRulesLit {
		NonterminalLit name;
		std::vector<ProdLit> prods;
	};

	template<typename TokenT = dpl::Token<>>
	struct LexLit {
		TerminalLit name;
		bool discard = false;
		dpl::Lexeme<char, TokenT> lex;
	};

	template<typename TokenT = dpl::Token<>>
	struct GrammarLit {
		std::vector<NtRulesLit> rules;
		std::vector<LexLit<TokenT>> lexemes;
	};

	namespace literals {
		constexpr NonterminalLit operator""nt(const char* str, size_t) {
			return NonterminalLit{ str };
		}

		constexpr TerminalLit operator""t(const char* str, size_t) {
			return TerminalLit{ str };
		}

		inline auto operator""tkn(const char* str, size_t) {
			return dpl::Token<>{ std::string_view{str}, std::string_view{str} };
		}
	}

	template<typename T>
	concept ProdLitSymbol =
		std::is_same_v<T, TerminalLit>
		|| std::is_same_v<T, NonterminalLit>;

	// symbol modifiers
	constexpr auto operator~(TerminalLit&& symbol) {
		symbol.modifier = TreeModifier::Hide;
		return symbol;
	}

	constexpr auto operator!(ProdLitSymbol auto&& symbol) {
		symbol.modifier = TreeModifier::Lift;
		return symbol;
	}

	constexpr auto operator*(NonterminalLit&& symbol) {
		symbol.modifier = TreeModifier::Adopt;
		return symbol;
	}

	// production modifiers
	auto operator&(ProdLit&& rule, Assoc assoc) {
		rule.assoc = assoc;
		return rule;
	}

	auto operator&(ProdLit&& rule, Prec prec) {
		rule.prec = prec;
		return rule;
	}

	constexpr auto operator&(const ProdLitSymbol auto& sym, Assoc assoc) {
		return ProdLit{ .sentence = {sym}, .assoc = assoc };
	}

	constexpr auto operator&(const ProdLitSymbol auto& sym, Prec prec) {
		return ProdLit{ .sentence = {sym}, .prec = prec };
	}

	// symbols list to production
	constexpr auto operator,(const ProdLitSymbol auto& lhs, const ProdLitSymbol auto& rhs) {
		return ProdLit{{ lhs, rhs }};
	}

	constexpr auto operator,(ProdLit&& lhs, const ProdLitSymbol auto& rhs) {
		lhs.sentence.push_back(rhs);
		return lhs;
	}

	template <typename T>
	concept RuleOrSingle = std::is_same_v<T, dpl::ProdLit>
		|| std::is_same_v<T, TerminalLit>
		|| std::is_same_v<T, NonterminalLit>;

	// productions list to nonterminal
	auto operator|=(const NonterminalLit& name, NtRulesLit&& nt) {
		nt.name = name;
		return nt;
	}

	auto operator|=(const NonterminalLit& name, ProdLit&& prod) {
		return NtRulesLit{ .name = name, .prods = {prod} };
	}

	auto operator|=(const NonterminalLit& name, ProdLitSymbol auto&& sym) {
		NtRulesLit rules { .name = name };
		ProdLit prod{};
		prod.sentence.push_back(sym);
		rules.prods.push_back(prod);
		return rules;
	}

	template<typename TokenT>
	constexpr auto operator|=(const TerminalLit& name, const dpl::Lexeme<char, TokenT>& lex) {
		return LexLit<TokenT>{.name = name, .lex = lex};
	}

	template<typename TokenT>
	constexpr auto operator|=(discard_type disc, const dpl::Lexeme<char, TokenT>& lex) {
		return LexLit<TokenT>{.discard = true, .lex = lex};
	}

	constexpr auto operator|=(const TerminalLit& name, const dpl::regex auto& re) {
		return LexLit<dpl::Token<>>{.name = name, .lex = dpl::Lexeme{ re }};
	}

	constexpr auto operator|=(discard_type disc, const dpl::regex auto& re) {
		return LexLit<dpl::Token<>>{.discard = true, .lex = dpl::Lexeme{ re }};
	}

	auto operator|(const ProdLit& lhs, const ProdLit& rhs) {
		NtRulesLit result;
		result.prods.push_back(lhs);
		result.prods.push_back(rhs);
		return result;
	}

	constexpr auto operator|(const ProdLit& lhs, const ProdLitSymbol auto& rhs) {
		NtRulesLit result;
		result.prods.push_back(lhs);
		result.prods.push_back(ProdLit{ .sentence = {rhs} });
		return result;
	}

	constexpr auto operator|(const ProdLitSymbol auto& lhs, const ProdLit& rhs) {
		NtRulesLit result;
		result.prods.push_back(ProdLit{ .sentence = {lhs} });
		result.prods.push_back(rhs);
		return result;
	}

	constexpr auto operator|(const ProdLitSymbol auto& lhs, const ProdLitSymbol auto& rhs) {
		NtRulesLit result;
		result.prods.push_back(ProdLit{ .sentence = {lhs} });
		result.prods.push_back(ProdLit{ .sentence = {rhs} });
		return result;
	}

	auto operator|(NtRulesLit&& lhs, const ProdLit& rhs) {
		lhs.prods.push_back(rhs);
		return lhs;
	}

	constexpr auto operator|(NtRulesLit&& lhs, const ProdLitSymbol auto& rhs) {
		lhs.prods.push_back(ProdLit{ .sentence{ rhs } });
		return lhs;
	}

	// nonterminals list to grammar
	auto operator,(const NtRulesLit& lhs, const NtRulesLit& rhs) {
		return GrammarLit<void_token_t>{ .rules = { lhs, rhs } };
	}

	template<typename TokenT>
	constexpr auto operator,(const NtRulesLit& lhs, const LexLit<TokenT>& rhs) {
		return GrammarLit<TokenT>{ .rules = { lhs }, .lexemes = { rhs } };
	}

	template<typename TokenT>
	constexpr auto operator,(const LexLit<TokenT>& lhs, const NtRulesLit& rhs) {
		return GrammarLit<TokenT>{ .rules = { rhs }, .lexemes = { lhs } };
	}

	template<typename TokenT>
	constexpr auto operator,(const LexLit<TokenT>& lhs, const LexLit<TokenT>& rhs) {
		return GrammarLit<TokenT>{ .lexemes = { lhs, rhs } };
	}

	template<typename T, typename TokenT>
	concept GrammarLitOfType =
		std::same_as<T, GrammarLit<void_token_t>>
		|| std::same_as<T, GrammarLit<TokenT>>;

	template<typename TokenT>
	constexpr auto operator,(GrammarLitOfType<TokenT> auto&& lhs, const LexLit<TokenT>& rhs) {
		lhs.lexemes.push_back(rhs);
		return lhs;
	}

	template<typename TokenT = dpl::Token<>>
	constexpr auto operator,(GrammarLitOfType<TokenT> auto&& lhs, const NtRulesLit& rhs) {
		lhs.rules.push_back(rhs);
		return lhs;
	}
}