#pragma once

#include <string_view>
#include <variant>
#include <vector>
#include <unordered_set>

#include "Token.h"

namespace dpl {

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class ProductionRule : private std::vector<std::variant<std::monostate, Token<KwdT, SymT>, std::string_view>> {
	public:

		using Token = Token<KwdT, SymT>;
		using Atom = std::variant<std::monostate, Token, std::string_view>;

		constexpr ProductionRule(std::initializer_list<Atom> l) : std::vector<Atom>(l) { }

		using std::vector<Atom>::size;
		using std::vector<Atom>::operator[];
		using std::vector<Atom>::begin;
		using std::vector<Atom>::end;
		using std::vector<Atom>::rbegin;
		using std::vector<Atom>::rend;

	};

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class Nonterminal : private std::vector<ProductionRule<KwdT, SymT>> {
	public:
		
		using ProductionRule = ProductionRule<KwdT, SymT>;

		constexpr Nonterminal() = default;
		constexpr Nonterminal(std::string_view n, std::initializer_list<ProductionRule> l) : std::vector<ProductionRule>(l), name(n) { }

		using std::vector<ProductionRule>::size;
		using std::vector<ProductionRule>::operator[];
		using std::vector<ProductionRule>::begin;
		using std::vector<ProductionRule>::end;

		std::string_view name;

	};

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class Grammar : std::unordered_map<std::string_view, Nonterminal<KwdT, SymT>> {
	public:

		using ProductionRule = ProductionRule<KwdT, SymT>;
		using Nonterminal = Nonterminal<KwdT, SymT>;
		using Token = Token<KwdT, SymT>;

		constexpr Grammar(std::initializer_list<Nonterminal> l) {
			std::for_each(l.begin(), l.end(), [&](const auto& e) {
				(*this)[e.name] = e;
			});
		}

		using std::unordered_map <std::string_view, Nonterminal>::size;
		using std::unordered_map <std::string_view, Nonterminal>::operator[];
		using std::unordered_map <std::string_view, Nonterminal>::begin;
		using std::unordered_map <std::string_view, Nonterminal>::end;

	};
}