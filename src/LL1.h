#pragma once

#include "Grammar.h"
#include "Token.h"
#include "ParseTree.h"

#include <unordered_map>
#include <algorithm>
#include <array>
#include <stack>

namespace dpl{
	template<typename KwdT, typename SymT>
	class LL1 {
	public:

		using Token = Token<KwdT, SymT>;
		using ProductionRule = ProductionRule<KwdT, SymT>;
		using Nonterminal = Nonterminal<KwdT, SymT>;
		using Grammar = Grammar<KwdT, SymT>;
		using ParseTree = ParseTree<KwdT, SymT>;
		using out_type = std::variant<Token, std::pair<std::string_view, int>>;

		LL1(Grammar& g, std::string_view s, ParseTree& pt) : grammar(g), start_symbol(s), out_tree(pt) {
			calcFirstSets();
			calcFollowSets();

			try {
				generateParseTable();
				parse_stack.push_back(TokenType::EndOfFile);
				parse_stack.push_back(start_symbol);
			} catch (const std::invalid_argument& err) {
				std::cerr << "LL(1) parser can't parse non-LL(1) grammar!\n";
			}
		}

		void calcFirstSets() {
			firsts.reserve(grammar.size());
			for (auto& [name, nt] : grammar) {
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

				for (auto& [name, nt] : grammar) {
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
			for (auto& [name, nt] : grammar) {
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

				for (auto& [name, nt] : grammar) {
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
			for (auto& [name, nt] : grammar) {
				for (int i = 0; i < nt.size(); i++) {
					auto& rule = nt[i];

					auto firsts_of_def = first_star(rule.begin(), rule.end());

					for (const auto& f : firsts_of_def) {
						try {
							if (const auto* t = std::get_if<Token>(&f)) {
								addEntry(*t, name, i);
							} else {
								std::for_each(follows[name].begin(), follows[name].end(), [&](const auto& e) {
									addEntry(e, name, i);
								});
							}
						} catch (const std::invalid_argument& err) {
							table.clear();
							throw err;
						}
					}
				}
			}
		}

		void operator<<(const Token& t) {
			bool terminal_eliminated = false;
			do {
				if (const auto* nontr = std::get_if<std::string_view>(&parse_stack.back())) {
					if (hasEntry(t, *nontr)) {

						auto& rule = grammar[*nontr][table[getTerminalType(t)][*nontr]];
						out_tree << std::make_pair(*nontr, table[getTerminalType(t)][*nontr]);

						parse_stack.pop_back();

						std::for_each(rule.rbegin(), rule.rend(), [&](const auto& e) {
							if (const auto* v = std::get_if<Token>(&e)) {
								parse_stack.push_back(*v);
							} else if (const auto* v = std::get_if<std::string_view>(&e)) {
								parse_stack.push_back(*v);
							}
						});

					} else {
						std::cerr << "Syntax error: unexpected token " << t << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
						return;
					}
				}

				if (const auto* tr = std::get_if<Token>(&parse_stack.back())) {
					if (*tr == t) {
						out_tree << t;
						parse_stack.pop_back();

						terminal_eliminated = true;

						if (parse_stack.empty()) return;
					} else {
						std::cerr << "Syntax error: unexpected token " << t << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
						return;
					}
				}
			} while (!terminal_eliminated);
		}

		bool hasEntry(const Token& tkn, std::string_view nontr) {
			return table.contains(getTerminalType(tkn)) && table[getTerminalType(tkn)].contains(nontr);
		}

		bool addEntry(const Token& tkn, std::string_view name, int i) {
			if (hasEntry(tkn, name)) throw std::invalid_argument("non LL(1) grammar");
			table[tkn][name] = i;
		}

		void printParseTable() {
			std::cout << "\n\nParse Table:\n";
			for (const auto& [tkn, row] : table) {
				std::cout << dpl::log::streamer{ tkn } << " :   ";
				for (const auto& [key, rule] : row) {
					std::cout << key << " (" << rule << ")  ";
				}
				std::cout << '\n';
			}
			std::cout << "\n\n";
		}

	private:

		std::string_view start_symbol;
		Grammar& grammar;

		std::unordered_map<std::string_view, std::unordered_set<std::variant<std::monostate, Token>>> firsts;
		std::unordered_map<std::string_view, std::unordered_set<Token>> follows;
		std::unordered_map<std::variant<std::monostate, Token>, std::unordered_map<std::string_view, int>> table;

		// #TASK : use list for debug, stack for release
		std::list<std::variant<std::string_view, Token>> parse_stack;

		ParseTree& out_tree;
	};
}