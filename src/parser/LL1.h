#pragma once

#include "../Grammar.h"
#include "../tokenizer/Token.h"
#include "../ParseTree.h"
#include "../tokenizer/Tokenizer.h"
#include "Parser.h"

#include <unordered_map>
#include <algorithm>
#include <array>
#include <stack>

namespace dpl{
	class LL1 : public Parser {
	public:

		using accept_action = std::monostate;
		using terminal_type = Terminal;
		using nonterminal_type = std::string_view;

		using out_type = std::variant<Token, std::pair<std::string_view, int>>;
		using symbol_type = std::variant<std::monostate, terminal_type>;	
		using table_type = std::unordered_map<symbol_type, std::unordered_map<nonterminal_type, int>>;

		out_type fetchNext() {
			next_node_ready = false;

			while (!next_node_ready && !input.closed()) {
				*this << input.fetchNext();
			}

			return next_node;
		}

		void operator<<(const Token& t_) {
			const terminal_type t = t_;

			bool terminal_eliminated = false;
			do {
				if (std::holds_alternative<nonterminal_type>(parse_stack.top())) {
					const auto nontr = std::get<nonterminal_type>(parse_stack.top());
					if (hasEntry(t, nontr)) {

						auto& rule = grammar[nontr][table[t][nontr]];

						std::cout << "Production out: (" << nontr << ", " << table[t][nontr] << ")\n";
						auto pair = std::make_pair(nontr, table[t][nontr]);
						this->tree_builder().pushNode(pair);

						parse_stack.pop();

						std::for_each(rule.rbegin(), rule.rend(), [&](const auto& e) {
							if (const auto* v = std::get_if<terminal_type>(&e)) {
								parse_stack.push(*v);
							} else if (const auto* v = std::get_if<nonterminal_type>(&e)) {
								parse_stack.push(*v);
							}
						});

					} else {
						std::cerr << "Syntax error: unexpected token " << t_ << " at position (" << dpl::log::streamer{ t_.pos } << ")\n";
						return;
					}
				}

				if (const auto* tr = std::get_if<terminal_type>(&parse_stack.top())) {
					if (*tr == t) {

						std::cout << "Token out: " << t.stringify() << '\n';
						this->tree_builder().pushNode(t_);
						parse_stack.pop();

						terminal_eliminated = true;

						if (parse_stack.empty()) {
							std::cout << "end of parsing\n";

							ParseTree out;
							this->tree_builder().assignToTree(out);
							std::cout << out;

							return;
						}
					} else {
						std::cerr << "Syntax error: unexpected token " << t_ << " at position (" << dpl::log::streamer{ t_.pos } << ")\n";
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

		const auto& generateParseTable() {
			table.clear();

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

			return table;
		}

		bool hasEntry(const terminal_type& tkn, nonterminal_type nontr) {
			return table.contains(tkn) && table[tkn].contains(nontr);
		}

		bool addEntry(const terminal_type& tkn, nonterminal_type name, int i) {
			if (hasEntry(tkn, name)) throw std::invalid_argument("non LL(1) grammar");
			table[tkn][name] = i;
		}

		LL1(Grammar& g, Tokenizer& inp) : Parser(g, inp), tb(g) {
			grammar.initialize();

			try {
				generateParseTable();
				parse_stack.push(Terminal::Type::EndOfFile);
				parse_stack.push(grammar.start_symbol);
			} catch (const std::invalid_argument& err) {
				std::cerr << "LL(1) parser can't accept non-LL(1) grammar!\n";
			}
		}

	private:

		TopDownTreeBuilder tb;
		TreeBuilder& tree_builder() {
			return tb;
		}
		
		out_type next_node;
		bool next_node_ready = false;

		table_type table;

		std::stack<std::variant<terminal_type, nonterminal_type>> parse_stack;
	};

	template<>
	const char* getParserName<LL1> = "LL(1)";
}