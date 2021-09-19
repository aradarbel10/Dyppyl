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

		using Token = Token<KwdT, SymT>;
		using Atom = std::variant<std::monostate, Token, std::string_view>;

		constexpr ProductionRule(std::initializer_list<Atom> l) : definition(l) { }

		constexpr auto& expand() { return definition; }

		#ifdef DPL_LOG
		template<typename KwdT, typename SymT>
		friend std::ostream& operator<<(std::ostream& os, const ProductionRule<KwdT, SymT>& t);
		#endif //DPL_LOG

		constexpr decltype(auto) size() {
			return definition.size();
		}

		constexpr decltype(auto) operator[](size_t i) {
			return definition[i];
		}

		constexpr auto begin() {
			return definition.begin();
		}

		constexpr auto end() {
			return definition.end();
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

		constexpr decltype(auto) size() {
			return nonterminals.size();
		}

		constexpr decltype(auto) operator[](std::string_view key) {
			return nonterminals[key];
		}

		constexpr auto begin() {
			return nonterminals.begin();
		}

		constexpr auto end() {
			return nonterminals.end();
		}

		friend class LL1<typename KwdT, typename SymT>;

	private:

		std::unordered_map<std::string_view, Nonterminal> nonterminals;

	};
}