#pragma once

#include <string_view>

namespace dpl {

	struct Nonterminal : public std::string_view {
		using std::string_view::string_view;
	};

	namespace literals {
		constexpr Nonterminal operator""nt(const char* str, size_t) {
			return Nonterminal{ str };
		}

		constexpr dpl::Terminal operator""t(const char* str, size_t) {
			return dpl::Terminal{ str };
		}
	}

	using Prec = short;

	constexpr auto operator&(ProductionRule&& rule, Assoc assoc) {
		rule.assoc = assoc;
		return rule;
	}

	constexpr auto operator&(ProductionRule&& rule, Prec prec) {
		rule.prec = prec;
		return rule;
	}

	template<typename T>
	concept RuleSymbol = std::is_same_v<T, Terminal> || std::is_same_v<T, Nonterminal>;

	constexpr auto operator,(const RuleSymbol auto& lhs, const RuleSymbol auto& rhs) {
		return ProductionRule{ lhs, rhs };
	}

	constexpr auto operator,(ProductionRule&& lhs, const RuleSymbol auto& rhs) {
		lhs.push_back(rhs);
		return lhs;
	}

	template <typename T>
	concept RuleOrSingle = std::is_same_v<T, dpl::ProductionRule>
		|| std::is_same_v<T, dpl::Terminal>
		|| std::is_same_v<T, dpl::Nonterminal>;

	constexpr auto operator|=(const Nonterminal& name, dpl::NonterminalRules&& nt) {
		nt.name = name;
		return nt;
	}

	constexpr auto operator|=(const Nonterminal& name, const RuleOrSingle auto& rule) {
		return NonterminalRules{ name, { rule } };
	}

	constexpr auto operator|(const RuleOrSingle auto& lhs, const RuleOrSingle auto& rhs) {
		return NonterminalRules{ "", {{lhs}, {rhs}} };
	}

	constexpr auto operator|(dpl::NonterminalRules&& nt, const RuleOrSingle auto& prod) {
		nt.push_back({ prod });
		return nt;
	}
}