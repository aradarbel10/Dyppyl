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

		constexpr Grammar(std::initializer_list<Nonterminal> l) : start_symbol((*l.begin()).name) {
			std::for_each(l.begin(), l.end(), [&](const auto& e) {
				(*this)[e.name] = e;
			});
		}

		void initialize() {
			calcFirstSets();
			calcFollowSets();
		}

		using std::unordered_map <std::string_view, Nonterminal>::size;
		using std::unordered_map <std::string_view, Nonterminal>::operator[];
		using std::unordered_map <std::string_view, Nonterminal>::begin;
		using std::unordered_map <std::string_view, Nonterminal>::end;
		using std::unordered_map <std::string_view, Nonterminal>::contains;


	public:

		std::unordered_map<std::string_view, std::unordered_set<std::variant<std::monostate, Token>>> firsts;
		std::unordered_map<std::string_view, std::unordered_set<Token>> follows;

		std::string_view start_symbol;

	private:

		void calcFirstSets() {
			firsts.reserve(size());
			for (auto& [name, nt] : (*this)) {
				firsts[name].reserve(nt.size());
				for (auto& rule : nt) {
					if (const Token* t = std::get_if<Token>(&rule[0])) {
						firsts[name].insert(*t);
					}

					if (rule.size() == 1) {
						if (std::holds_alternative<std::monostate>(rule[0])) {
							firsts[name].insert(std::monostate());
						}
					}
				}
			}

			bool changed;
			do {
				changed = false;

				for (auto& [name, nt] : (*this)) {
					for (auto& rule : nt) {
						auto size_before = firsts[name].size();

						int i = 0;
						for (i = 0; i < rule.size(); i++) {
							if (const auto* v = std::get_if<Token>(&rule[i])) {
								firsts[name].insert(*v);
								break;
							} else if (const auto* v = std::get_if<std::string_view>(&rule[i])) {
								if (!firsts[*v].contains(std::monostate())) {
									std::for_each(firsts[*v].begin(), firsts[*v].end(), [&](const auto& e) {
										firsts[name].insert(e);
									});
									break;
								}
							}
						}

						if (i == rule.size()) {
							firsts[name].insert(std::monostate());
						}

						if (size_before != firsts[name].size()) changed = true;
					}
				}
			} while (changed);
		}

		void calcFollowSets() {
			for (auto& [name, nt] : (*this)) {
				for (auto& rule : nt) {
					for (int i = 0; i < rule.size() - 1; i++) {
						if (const auto* n = std::get_if<std::string_view>(&rule[i])) {
							if (const auto* t = std::get_if<Token>(&rule[i + 1])) {
								follows[*n].insert(*t);
							}
						}
					}
				}
			}

			follows[start_symbol].insert(TokenType::EndOfFile);

			bool changed;
			do {
				changed = false;

				for (auto& [name, nt] : (*this)) {
					for (auto& rule : nt) {
						for (int i = 0; i < rule.size() - 1; i++) {
							if (const auto* v = std::get_if<std::string_view>(&rule[i])) {
								auto size_before = follows[*v].size();

								const auto first_of_rest = first_star(std::next(rule.begin(), i + 1), rule.end());
								bool contains_epsilon = false;

								std::for_each(first_of_rest.begin(), first_of_rest.end(), [&](const auto& e) {
									if (std::holds_alternative<std::monostate>(e)) {
										contains_epsilon = true;
									} else {
										follows[*v].insert(std::get<Token>(e));
									}
								});

								std::for_each(follows[name].begin(), follows[name].end(), [&](const auto& e) {
									follows[*v].insert(e);
								});

								if (size_before != follows[*v].size()) changed = true;
							}
						}
					}
				}

			} while (changed);
		}

	public:

		template<class InputIt> requires std::input_iterator<InputIt>
		std::unordered_set<std::variant<std::monostate, Token>> first_star(InputIt first, InputIt last) {
			if (first == last || (std::distance(first, last) == 1 && std::holds_alternative<std::monostate>(*first))) {
				return { std::monostate() };
			}

			if (const auto* v = std::get_if<Token>(&(*first))) {
				return { *v };
			}

			if (const auto* v = std::get_if<std::string_view>(&(*first))) {
				auto result = firsts[*v];

				if (firsts[*v].contains(std::monostate())) {
					result.erase(std::monostate());
					auto rest = first_star(std::next(first), last);

					std::for_each(rest.begin(), rest.end(), [&](const auto& e) {
						result.insert(e);
					});

					return result;
				} else {
					return result;
				}
			}
		}

	};
}