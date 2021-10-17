#pragma once

#include "../Grammar.h"
#include "../tokenizer/Token.h"
#include "../ParseTree.h"
#include "../tokenizer/Tokenizer.h"

#include <unordered_map>
#include <algorithm>
#include <array>
#include <stack>

namespace dpl{
	class LL1 {
	public:

		using accept_action = std::monostate;
		using terminal_type = Terminal;
		using nonterminal_type = std::string_view;

		using out_type = std::variant<Token, std::pair<std::string_view, int>>;
		using symbol_type = std::variant<std::monostate, terminal_type>;		

		LL1(Grammar& g, ParseTree& pt, Tokenizer& inp) : input(inp), grammar(g), out_tree(pt) {
			grammar.initialize();

			try {
				generateParseTable();
				parse_stack.push_back(Token::Type::EndOfFile);
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
				if (const auto* nontr = std::get_if<nonterminal_type>(&parse_stack.back())) {
					if (hasEntry(t, *nontr)) {

						auto& rule = grammar[*nontr][table[t][*nontr]];

						std::cout << "Production out: (" << *nontr << ", " << table[t][*nontr] << ")\n";
						out_tree << std::make_pair(*nontr, table[t][*nontr]);

						parse_stack.pop_back();

						std::for_each(rule.rbegin(), rule.rend(), [&](const auto& e) {
							if (const auto* v = std::get_if<terminal_type>(&e)) {
								parse_stack.push_back(*v);
							} else if (const auto* v = std::get_if<nonterminal_type>(&e)) {
								parse_stack.push_back(*v);
							}
						});

					} else {
						std::cerr << "Syntax error: unexpected token " << t << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
						return;
					}
				}

				if (const auto* tr = std::get_if<terminal_type>(&parse_stack.back())) {
					if (*tr == t) {

						std::cout << "Token out: " << t.stringify() << '\n';
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
			//std::cout << "\n\nParse Table:\n";
			//for (const auto& [tkn, row] : table) {
			//	std::cout << tkn << " :   ";
			//	for (const auto& [key, rule] : row) {
			//		std::cout << key << " (" << rule << ")  ";
			//	}
			//	std::cout << '\n';
			//}
			//std::cout << "\n\n";
		}

	private:

		void generateParseTable() {
			for (auto& [name, nt] : grammar) {
				for (int i = 0; i < nt.size(); i++) {
					auto& rule = nt[i];

					auto firsts_of_def = grammar.first_star(rule.begin(), rule.end());

					for (const auto& f : firsts_of_def) {
						try {
							if (const auto* t = std::get_if<terminal_type>(&f)) {
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

		bool hasEntry(const terminal_type& tkn, nonterminal_type nontr) {
			return table.contains(tkn) && table[tkn].contains(nontr);
		}

		bool addEntry(const terminal_type& tkn, nonterminal_type name, int i) {
			if (hasEntry(tkn, name)) throw std::invalid_argument("non LL(1) grammar");
			table[tkn][name] = i;
		}

	private:

		Tokenizer& input;
		out_type next_node;
		bool next_node_ready = false;

		Grammar& grammar;

		std::unordered_map<symbol_type, std::unordered_map<nonterminal_type, int>> table;

		// #TASK : use list for debug, stack for release
		std::list<std::variant<terminal_type, nonterminal_type>> parse_stack;

		// #TASK : use tree builder in LL1
		ParseTree& out_tree;
	};
}