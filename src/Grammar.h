#pragma once

#include <string_view>
#include <variant>
#include <vector>
#include <unordered_set>

#include "Token.h"

namespace dpl {

	template<typename KwdT, typename SymT> class LL1;

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class Nonterminal;

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT>&& std::is_enum_v<SymT>
	class Grammar;

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class ProductionRule {
	public:

		enum class Special { Epsilon, Self };

		using Token = Token<KwdT, SymT>;
		using Atom = std::variant<Special, Token, std::string_view>;

		constexpr ProductionRule(std::initializer_list<Atom> l) : definition(l) {
			//std::replace_if(definition.begin(), definition.end(), [](const Atom& a) {
			//	return std::holds_alternative<Self>(a);
			//}, this);
		}

		constexpr auto& expand() { return definition; }

		template<typename KwdT, typename SymT>
		friend std::ostream& operator<<(std::ostream& os, const ProductionRule<KwdT, SymT>& t);

		constexpr decltype(auto) size() {
			return definition.size();
		}

		constexpr decltype(auto) operator[](size_t i) {
			return definition[i];
		}

		constexpr std::vector<Atom>& getDefinition() {
			return definition;
		}

	private:

		std::vector<Atom> definition;

	};

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class Nonterminal {
	public:
		
		using ProductionRule = ProductionRule<KwdT, SymT>;

		constexpr Nonterminal() = default;
		constexpr Nonterminal(std::string_view n, std::initializer_list<ProductionRule> l) : productions(l), name(n) { }

		template<typename KwdT, typename SymT>
		friend std::ostream& operator<<(std::ostream& os, const Nonterminal<KwdT, SymT>& t);

		constexpr decltype(auto) size() {
			return productions.size();
		}

		constexpr decltype(auto) operator[](size_t i) {
			return productions[i];
		}

		constexpr std::vector<ProductionRule>& getProductions() {
			return productions;
		}

		constexpr std::string_view getName() const {
			return name;
		}

		friend class LL1<typename KwdT, typename SymT>;
		friend class Grammar<typename KwdT, typename SymT>;

	private:

		std::vector<ProductionRule> productions;
		std::string_view name;

	};

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class Grammar {
	public:

		using ProductionRule = ProductionRule<KwdT, SymT>;
		using Nonterminal = Nonterminal<KwdT, SymT>;
		using Token = Token<KwdT, SymT>;

		constexpr Grammar(std::initializer_list<Nonterminal> l) {
			std::for_each(l.begin(), l.end(), [&](const auto& e) {
				nonterminals[e.name] = e;
			});
		}

		friend class LL1<typename KwdT, typename SymT>;

	private:

		std::unordered_map<std::string_view, Nonterminal> nonterminals;

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

			if (const auto* val = std::get_if<0>(&e)) {
				os << magic_enum::enum_name<ProductionRule<KwdT, SymT>::Special>(*val);
				continue;
			}

			if (const auto* val = std::get_if<1>(&e)) {
				os << *val;
				continue;
			}

			if (const auto* val = std::get_if<2>(&e)) {
				os << "[nonterminal, " << *val << "]";
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