#pragma once

#include "Grammar.h"
#include "Token.h"
#include "ParseTree.h"
#include "Tokenizer.h"

#include <unordered_map>
#include <algorithm>
#include <array>
#include <stack>

namespace dpl{
	template<typename KwdT, typename SymT>
	class LL1 {
	public:

		using Tokenizer = dpl::Tokenizer<KwdT, SymT>;
		using Token = Token<KwdT, SymT>;
		using ProductionRule = ProductionRule<KwdT, SymT>;
		using Nonterminal = Nonterminal<KwdT, SymT>;
		using Grammar = Grammar<KwdT, SymT>;
		using ParseTree = ParseTree<KwdT, SymT>;
		using out_type = std::variant<Token, std::pair<std::string_view, int>>;
		

		LL1(Grammar& g, ParseTree& pt, Tokenizer& inp) : input(inp), grammar(g), out_tree(pt) {
			grammar.initialize();

			try {
				generateParseTable();
				parse_stack.push_back(TokenType::EndOfFile);
				parse_stack.push_back(grammar.start_symbol);
			} catch (const std::invalid_argument& err) {
				std::cerr << "LL(1) parser can't parse non-LL(1) grammar!\n";
			}
		}

		out_type fetchNext() {
			next_node_ready = false;

			while (!next_node_ready && !input.closed()) {
				*this << input.fetchNext();
			}

			return next_node;
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

		void generateParseTable() {
			for (auto& [name, nt] : grammar) {
				for (int i = 0; i < nt.size(); i++) {
					auto& rule = nt[i];

					auto firsts_of_def = grammar.first_star(rule.begin(), rule.end());

					for (const auto& f : firsts_of_def) {
						try {
							if (const auto* t = std::get_if<Token>(&f)) {
								addEntry(*t, name, i);
							} else {
								std::for_each(grammar.follows[name].begin(), grammar.follows[name].end(), [&](const auto& e) {
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

		bool hasEntry(const Token& tkn, std::string_view nontr) {
			return table.contains(getTerminalType(tkn)) && table[getTerminalType(tkn)].contains(nontr);
		}

		bool addEntry(const Token& tkn, std::string_view name, int i) {
			if (hasEntry(tkn, name)) throw std::invalid_argument("non LL(1) grammar");
			table[tkn][name] = i;
		}

	private:

		Tokenizer& input;
		out_type next_node;
		bool next_node_ready = false;

		Grammar& grammar;

		std::unordered_map<std::variant<std::monostate, Token>, std::unordered_map<std::string_view, int>> table;

		// #TASK : use list for debug, stack for release
		std::list<std::variant<std::string_view, Token>> parse_stack;

		ParseTree& out_tree;
	};
}