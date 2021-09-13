#pragma once

#include <string_view>
#include <variant>
#include <vector>
#include <array>

#include "Token.h"

namespace dpl {

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT>&& std::is_enum_v<SymT>
	class Nonterminal;

	struct Self {};

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class ProductionRule {
	public:

		using Token = Token<KwdT, SymT>;
		using Atom = std::variant<std::monostate, Self, Token, Nonterminal<KwdT, SymT>*>;
		
		constexpr ProductionRule(std::initializer_list<Atom> l) : definition(l) {
			//std::replace_if(definition.begin(), definition.end(), [](const Atom& a) {
			//	return std::holds_alternative<Self>(a);
			//}, this);
		}

		constexpr auto& expand() { return definition; }

		template<typename KwdT, typename SymT>
		friend std::ostream& operator<<(std::ostream& os, const ProductionRule<KwdT, SymT>& t);

	private:

		std::vector<Atom> definition;

	};

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class Nonterminal {
	public:
		
		using ProductionRule = ProductionRule<KwdT, SymT>;

		constexpr Nonterminal(std::initializer_list<ProductionRule> l) : productions(l) { }

		template<typename KwdT, typename SymT>
		friend std::ostream& operator<<(std::ostream& os, const Nonterminal<KwdT, SymT>& t);

	private:

		std::vector<ProductionRule> productions;

	};

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class Grammar {
	public:

		using Nonterminal = Nonterminal<KwdT, SymT>;

		constexpr Grammar(std::initializer_list<Nonterminal> l) : nonterminals(l) { }

	private:

		std::vector<Nonterminal> nonterminals;

	};

	// #TASK : implement "+|" syntax for productions and nonterminals:
	// Nonterminal = (terminal + terminal + terminal)
	//			   | (terminal + terminal)
	//			   | (terminal + terminal + terminal + terminal)
	//			   ...

	template<typename KwdT, typename SymT>
	std::ostream& operator<<(std::ostream& os, const ProductionRule<KwdT, SymT>& t) {
		for (const auto& e : t.definition) {
			os << ' ';

			const auto* val0 = std::get_if<0>(&e);
			if (val0) {
				os << 'e';
				continue;
			}

			const auto* val1 = std::get_if<1>(&e);
			if (val1) {
				os << "[self]";
				continue;
			}

			const auto* val2 = std::get_if<2>(&e);
			if (val2) {
				os << *val2;
				continue;
			}

			const auto* val3 = std::get_if<3>(&e);
			if (val3) {
				os << "[nonterminal]";
			}
		}

		return os;
	}

	template<typename KwdT, typename SymT>
	std::ostream& operator<<(std::ostream& os, const Nonterminal<KwdT, SymT>& t) {
		for (const auto& e : t.productions) {
			os << e << '\n';
		}

		return os;
	}
}