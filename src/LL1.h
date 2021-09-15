#pragma once

#include "Grammar.h"

#include <unordered_map>
#include <algorithm>
#include <array>

namespace dpl{

	template<typename KwdT, typename SymT>
	class LL1 {
	public:

		using Token = Token<KwdT, SymT>;
		using ProductionRule = ProductionRule<KwdT, SymT>;
		using Nonterminal = Nonterminal<KwdT, SymT>;
		using Grammar = Grammar<KwdT, SymT>;

		LL1(Grammar& g, std::string_view s) : grammar(g), start_symbol(s) {
			calcFirstSets();
			calcFollowSets();
			generateParseTable();
		}

		void calcFirstSets() {
			firsts.reserve(grammar.nonterminals.size());
			for (auto& [name, nt] : grammar.nonterminals) {
				firsts[name].reserve(nt.size());
				for (auto& rule : nt.getProductions()) {
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

				for (auto& [name, nt] : grammar.nonterminals) {
					for (auto& rule : nt.productions) {
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
			for (auto& [name, nt] : grammar.nonterminals) {
				for (auto& rule : nt.productions) {
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

				for (auto& [name, nt] : grammar.nonterminals) {
					for (auto& rule : nt.productions) {
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

		void generateParseTable() {
			for (auto& [name, nt] : grammar.nonterminals) {
				for (int i = 0; i < nt.productions.size(); i++) {
					auto& rule = nt.productions[i];

					auto firsts_of_def = first_star(rule.begin(), rule.end());

					for (const auto& f : firsts_of_def) {
						if (const auto* t = std::get_if<Token>(&f)) {
							table[*t][name] = i;
						} else {
							std::for_each(follows[name].begin(), follows[name].end(), [&](const auto& e) {
								table[e][name] = i;
							});
						}
					}

					//if (const auto* v = std::get_if<Token>(&rule[0])) {
					//	table[*v][name] = i;
					//} else if (const auto* v = std::get_if<std::string_view>(&rule[0])) {
					//	std::for_each(firsts[*v].begin(), firsts[*v].end(), [&](const auto& t) {
					//		table[t][name] = i;
					//	});
					//}
				}
			}

			for (auto& [terminal, row] : table) {
				if (const auto* v = std::get_if<Token>(&terminal)) {
					std::cout << *v << ": ";
				} else {
					std::cout << "$: ";
				}

				for (auto& [cell, rulenum] : row) {
					std::cout << '(' << cell << ", " << rulenum << ") ";
				}
				std::cout << '\n';
			}
		}

	private:

		std::string_view start_symbol;
		Grammar& grammar;

		std::unordered_map<std::string_view, std::unordered_set<std::variant<std::monostate, Token>>> firsts;
		std::unordered_map<std::string_view, std::unordered_set<Token>> follows;
		std::unordered_map<std::variant<std::monostate, Token>, std::unordered_map<std::string_view, int>> table;
	};
}