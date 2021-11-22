#pragma once

#include "../Grammar.h"
#include "../tokenizer/Token.h"
#include "../ParseTree.h"
#include "../tokenizer/Tokenizer.h"
#include "Parser.h"

#include <algorithm>
#include <array>
#include <stack>

namespace dpl{

	class LLTable : private dpl::cc::map<std::pair<Terminal, std::string_view>, int> {
	private:

		bool is_ll1 = true;

	public:

		dpl::Grammar& grammar;

		using terminal_type = Terminal;
		using nonterminal_type = std::string_view;
		using table_type = dpl::cc::map<std::pair<terminal_type, nonterminal_type>, int>;

		using table_type::map;
		using table_type::operator[];
		using table_type::contains;
		using table_type::operator=;
		using table_type::begin;
		using table_type::end;
		using table_type::operator==;
		using table_type::size;
		using table_type::insert;

		constexpr LLTable(dpl::Grammar& g) : grammar(g) {
			grammar.initialize();
			try {
				generate();
			} catch (const std::invalid_argument&) {
				std::cerr << "LL(1) parser can't accept non-LL(1) grammar!\n";
			}
		}

		constexpr LLTable(dpl::Grammar& g, table_type& t) : grammar(g), table_type(t) {
			grammar.initialize();
		}

		constexpr LLTable(dpl::Grammar& g, dpl::cc::map<terminal_type, dpl::cc::map<nonterminal_type, int>> other) : grammar(g) {
			for (const auto& [terminal, row] : other) {
				for (const auto& [nonterminal, val] : row) {
					// #TASK : what if there are duplicates?
					insert({terminal, nonterminal}, val);
				}
			}
		}


		constexpr bool compareTo(const LLTable& other) const {
			if (size() != other.size()) return false;

			for (const auto& [pair, val] : *this) {
				if (val != other.at(pair)) {
					return false;
				}
			}

			return true;
		}

		constexpr void generate() {
			clear();

			for (auto& [name, nt] : grammar) {
				for (int i = 0; i < nt.size(); i++) {
					auto& rule = nt[i];

					auto firsts_of_def = grammar.first_star(rule);

					for (const auto& f : firsts_of_def) {
						try {
							if (const auto* tkn = std::get_if<terminal_type>(&f)) {
								if (contains({*tkn, name})) throw std::invalid_argument("non LL(1) grammar");

								insert({*tkn, name}, i);
							} else {
								for (auto iter = grammar.follows.at(name).begin(); iter != grammar.follows.at(name).end(); iter++) {
									if (contains({*iter, name})) throw std::invalid_argument("non LL(1) grammar");

									insert({*iter, name}, i);
								}
							}
						} catch (const std::invalid_argument& err) {
							is_ll1 = false;
							throw err; // rethrow
						}
					}
				}
			}
		}

		bool isLL1() const { return is_ll1; }

	};


	class LL1 : public Parser {
	public:

		using accept_action = std::monostate;
		using terminal_type = Terminal;
		using nonterminal_type = std::string_view;

		using out_type = std::variant<Token, RuleRef>;
		using symbol_type = std::variant<terminal_type, nonterminal_type>;
		using table_type = dpl::cc::map<terminal_type, dpl::cc::map<nonterminal_type, int>>;

		void operator<<(const Token& t_) {
			const terminal_type t = t_;

			bool terminal_eliminated = false;
			do {
				if (std::holds_alternative<nonterminal_type>(parse_stack.top())) {
					const auto nontr = std::get<nonterminal_type>(parse_stack.top());
					if (hasEntry(t, nontr)) {

						auto& rule = grammar[nontr][table[{t, nontr}]];

						std::cout << "Production out: (" << nontr << ", " << table[{t, nontr}] << ")\n";
						this->tree_builder().pushNode(RuleRef{ grammar, nontr, table[{t, nontr}] });

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

							this->tree_builder().assignToTree(out_tree);

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

		const auto& getParseTable() { return table; }

		bool hasEntry(const terminal_type& tkn, nonterminal_type nontr) {
			return table.contains({tkn, nontr});
		}

		bool addEntry(const terminal_type& tkn, nonterminal_type name, int i) {
			if (hasEntry(tkn, name)) throw std::invalid_argument("non LL(1) grammar");
			table.insert({ tkn, name }, i);
			return false; // returns whether entry already existed
		}

		LL1(Grammar& g) : Parser(g), tb(g), table(g) { }
		LL1(const LLTable& t) : table(t), Parser(t.grammar), tb(t.grammar) { }

		void parse_init() override {
			std::stack<symbol_type>{}.swap(parse_stack); // clear stack

			parse_stack.push(Terminal::Type::EndOfFile);
			parse_stack.push(grammar.start_symbol);
		}

	private:

		TopDownTreeBuilder tb;
		TreeBuilder& tree_builder() { return tb; }
		
		out_type next_node;
		bool next_node_ready = false;

		LLTable table;

		std::stack<symbol_type> parse_stack;
	};

	template<>
	const char* getParserName<LL1> = "LL(1)";
}