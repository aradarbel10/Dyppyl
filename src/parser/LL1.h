#pragma once

#include "../Grammar.h"
#include "../tokenizer/Token.h"
#include "../ParseTree.h"
#include "../tokenizer/Tokenizer.h"
#include "Parser.h"

#include <algorithm>
#include <array>
#include <stack>

#include <hybrid/hybrid.hpp>

namespace dpl{

	template<typename GrammarT = dpl::Grammar<>>
	class LLTable : private hybrid::map<std::pair<typename GrammarT::terminal_type, typename GrammarT::nonterminal_type>, int> {
	public:
		using grammar_type = GrammarT;

		using terminal_type = GrammarT::temrinal_type;
		using nonterminal_type = GrammarT::nontemrinal_type;
		using table_type = hybrid::map<std::pair<terminal_type, nonterminal_type>, int>;

	private:
		grammar_type& grammar;
		bool is_ll1 = true;

	public:

		using table_type::map;
		using table_type::operator[];
		using table_type::at;
		using table_type::contains;
		using table_type::operator=;
		using table_type::begin;
		using table_type::end;
		using table_type::size;
		using table_type::insert;
		using table_type::clear;

		constexpr LLTable(grammar_type& g) : grammar(g) {
			grammar.initialize();
			try {
				generate();
			} catch (const std::invalid_argument&) {
				std::cerr << "LL(1) parser can't accept non-LL(1) grammar!\n";
			}
		}

		constexpr LLTable(grammar_type& g, table_type& t) : grammar(g), table_type(t) {
			grammar.initialize();
		}

		constexpr LLTable(grammar_type& g, dpl::cc::map<terminal_type, dpl::cc::map<nonterminal_type, int>> other) : grammar(g) {
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
				if (val != other[pair]) {
					return false;
				}
			}

			return true;
		}

		constexpr void generate() {
			is_ll1 = true;
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

	template<typename GrammarT = dpl::Grammar<>>
	class LL1 : public Parser<GrammarT> {
	public:
		using grammar_type = GrammarT;

		using accept_action = std::monostate;
		using terminal_type = grammar_type::terminal_type;
		using token_type = grammar_type::token_type;
		using nonterminal_type = grammar_type::nonterminal_type;

		using out_type = std::variant<token_type, RuleRef<grammar_type>>;
		using symbol_type = std::variant<terminal_type, nonterminal_type>;
		using table_type = dpl::cc::map<terminal_type, dpl::cc::map<nonterminal_type, int>>;

	private:
		TopDownTreeBuilder<grammar_type> tb;
		TreeBuilder<grammar_type>& tree_builder() { return tb; }

		out_type next_node;
		bool next_node_ready = false;

		LLTable<grammar_type> table;

		std::stack<symbol_type> parse_stack;

	private:

		void operator<<(const token_type& t_) {
			const terminal_type t = t_;

			if (this->options.error_mode == Parser<grammar_type>::Options::ErrorMode::Panic && !this->fixed_last_error) {
				if (sync_set().contains(t)) {
					parse_stack.pop();
					this->fixed_last_error = true;
				} else {
					return;
				}
			}

			bool terminal_eliminated = false;
			do {
				if (parse_stack.empty()) {
					err_unexpected_token(t_);
					return;
				}

				if (std::holds_alternative<nonterminal_type>(parse_stack.top())) {
					const auto nontr = std::get<nonterminal_type>(parse_stack.top());
					if (table.contains({ t, nontr })) {

						auto& rule = this->grammar[nontr][table[{t, nontr}]];

						if (this->options.log_step_by_step)
							this->options.logprintln("Parser Trace", "Production out: (", nontr, ", ", table[{t, nontr}], ")");

						this->tree_builder().pushNode(RuleRef{ this->grammar, nontr, table[{t, nontr}] });

						parse_stack.pop();

						std::for_each(rule.rbegin(), rule.rend(), [&](const auto& e) {
							if (const auto* v = std::get_if<terminal_type>(&e)) {
								parse_stack.push(*v);
							} else if (const auto* v = std::get_if<nonterminal_type>(&e)) {
								parse_stack.push(*v);
							}
						});

					} else {
						err_unexpected_token(t_);
						return;
					}
				}

				if (const auto* tr = std::get_if<terminal_type>(&parse_stack.top())) {
					if (*tr == t) {

						if (this->options.log_step_by_step)
							this->options.logprintln("Parser Trace", "Token out: ", t.stringify());

						this->tree_builder().pushNode(t_);
						parse_stack.pop();

						terminal_eliminated = true;

						if (parse_stack.empty()) {
							if (this->options.log_step_by_step)
								this->options.logprintln("Parser Trace", "end of parsing");

							this->tree_builder().assignToTree(this->out_tree);

							return;
						}
					} else {
						err_unexpected_token(t_);
						return;
					}
				}
			} while (!terminal_eliminated);
		}

		const auto& getParseTable() { return table; }

		LL1(grammar_type& g, const Parser<grammar_type>::Options& ops = {}) : Parser(g, ops), tb(g), table(g) { }
		LL1(const LLTable<grammar_type>& t, const Parser<grammar_type>::Options& ops = {}) : table(t), Parser(t.grammar, ops), tb(t.grammar) { }

		void parse_init() override {
			std::stack<symbol_type>{}.swap(parse_stack); // clear stack

			parse_stack.push(Terminal::Type::EndOfFile);
			parse_stack.push(this->grammar.start_symbol);
		}
		
		std::set<terminal_type> currently_expected_terminals() const {
			if (parse_stack.empty()) {
				return { {dpl::Terminal::Type::EndOfFile} };
			}

			if (const auto* terminal = std::get_if<terminal_type>(&parse_stack.top())) {
				return { *terminal };
			} else {
				const auto& nonterminal = std::get<nonterminal_type>(parse_stack.top());
				std::set<terminal_type> result;

				dpl::ProductionRule stack_beg;

				auto iter = parse_stack._Get_container().rbegin();
				do {
					stack_beg.push_back(*iter);
					iter++;
				} while (iter != parse_stack._Get_container().rend() && !std::holds_alternative<terminal_type>(*std::prev(iter)));

				for (const auto& terminal : this->grammar.first_star(stack_beg)) {
					result.insert(std::get<terminal_type>(terminal));
				}

				return result;
			}
		}

		std::set<terminal_type> sync_set() const {
			if (const auto* nonterminal = std::get_if<nonterminal_type>(&parse_stack.top())) {
				std::set<terminal_type> result;
				for (const auto& symbol : this->grammar.follows[*nonterminal]) {
					result.insert(symbol);
				}
				return result;
			} else {
				return { std::get<terminal_type>(parse_stack.top()) };
			}
		}
	};
}