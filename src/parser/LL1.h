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

	template<typename GrammarT = dpl::Grammar<>>
	class LLTable : private std::map<std::pair<typename GrammarT::terminal_type, typename GrammarT::nonterminal_type>, int> {
	public:
		using grammar_type = GrammarT;

		using terminal_type = GrammarT::terminal_type;
		using nonterminal_type = GrammarT::nonterminal_type;
		using table_type = std::map<std::pair<terminal_type, nonterminal_type>, int>;

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
			generate();
		}

		constexpr LLTable(grammar_type& g, table_type& t) : grammar(g), table_type(t) {
			grammar.initialize();
		}

		constexpr LLTable(grammar_type& g, std::map<terminal_type, std::map<nonterminal_type, int>> other) : grammar(g) {
			for (const auto& [terminal, row] : other) {
				for (const auto& [nonterminal, val] : row) {
					// #TASK : what if there are duplicates?
					if (contains({ terminal, nonterminal })) throw std::exception{}; // duplicate entires - grammar provided is not LL1!
					insert({ {terminal, nonterminal}, val });
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
			is_ll1 = true;
			clear();

			for (auto& [name, nt] : grammar) {
				for (int i = 0; i < nt.size(); i++) {
					auto& rule = nt[i];

					auto firsts_of_def = grammar.first_star(rule);

					for (const auto& f : firsts_of_def) {
						if (const auto* tkn = std::get_if<terminal_type>(&f)) {
							if (contains({ *tkn, name })) {
								is_ll1 = false;
								throw std::invalid_argument("non LL(1) grammar");
							}

							insert({ { *tkn, name }, i });
						} else {
							for (auto iter = grammar.get_follows().at(name).begin(); iter != grammar.get_follows().at(name).end(); iter++) {
								if (contains({ *iter, name })) {
									is_ll1 = false;
									throw std::invalid_argument("non LL(1) grammar");
								}

								insert({ { *iter, name }, i });
							}
						}
					}
				}
			}
		}

		bool isLL1() const { return is_ll1; }

	};

	template<typename GrammarT = dpl::Grammar<>, typename LexiconT = dpl::Lexicon<>>
	class LL1 : public Parser<GrammarT, LexiconT> {
	public:
		using grammar_type = GrammarT;
		using lexicon_type = LexiconT;

		using accept_action = std::monostate;
		using terminal_type = grammar_type::terminal_type;
		using token_type = grammar_type::token_type;
		using nonterminal_type = grammar_type::nonterminal_type;

		using out_type = std::variant<token_type, RuleRef<grammar_type>>;
		using symbol_type = std::variant<terminal_type, nonterminal_type>;
		using table_type = std::map<terminal_type, std::map<nonterminal_type, int>>;

	private:
		TopDownTreeBuilder<grammar_type> tree_builder;

		out_type next_node;
		bool next_node_ready = false;

		LLTable<grammar_type> table;

		std::deque<symbol_type> parse_stack;

	public:
		LL1(grammar_type& g, lexicon_type& l, const typename Parser<grammar_type, lexicon_type>::Options& ops = {})
			: Parser<grammar_type, lexicon_type>(g, l, ops), tree_builder(g), table(g) { }

		LL1(const LLTable<grammar_type>& t, lexicon_type& l, const typename Parser<grammar_type, lexicon_type>::Options& ops = {})
			: table(t), Parser<grammar_type, lexicon_type>(t.grammar, l, ops), tree_builder(t.grammar) { }

		const auto& getParseTable() { return table; }

	private:

		void operator<<(const token_type& t_) {
			const terminal_type t = t_;

			if (this->options.error_mode == ErrorMode::Panic && !this->fixed_last_error) {
				if (sync_set().contains(t)) {
					parse_stack.pop_back();
					this->fixed_last_error = true;
				} else {
					return;
				}
			}

			bool terminal_eliminated = false;
			do {
				if (parse_stack.empty()) {
					this->err_unexpected_token(t_);
					return;
				}

				if (std::holds_alternative<nonterminal_type>(parse_stack.back())) {
					const auto nontr = std::get<nonterminal_type>(parse_stack.back());
					if (table.contains({ t, nontr })) {

						auto& rule = this->grammar[nontr][table[{t, nontr}]];

						if (this->options.log_step_by_step)
							this->options.logprintln("Parser Trace", "Production out: (", nontr, ", ", table[{t, nontr}], ")");

						tree_builder.pushNode(RuleRef{ this->grammar, nontr, table[{t, nontr}] });
						parse_stack.pop_back();
						
						std::for_each(rule.rbegin(), rule.rend(), [&](const auto& e) {
							if (const auto* v = std::get_if<terminal_type>(&e)) {
								parse_stack.push_back(*v);
							} else if (const auto* v = std::get_if<nonterminal_type>(&e)) {
								parse_stack.push_back(*v);
							}
						});

					} else {
						this->err_unexpected_token(t_);
						return;
					}
				}

				if (const auto* tr = std::get_if<terminal_type>(&parse_stack.back())) {
					if (*tr == t) {
						terminal_eliminated = true;
						parse_stack.back();

						if (tr->type == terminal_type::Type::eof) {
							if (this->options.log_step_by_step)
								this->options.logprintln("Parser Trace", "end of parsing");

							tree_builder.pushNode(*tr);
							tree_builder.assignToTree(this->out_tree);

							return;
						} else {
							if (this->options.log_step_by_step)
								this->options.logprintln("Parser Trace", "Token out: ", t.stringify());

							tree_builder.pushNode(t_);
						}
					} else {
						this->err_unexpected_token(t_);
						return;
					}
				}
			} while (!terminal_eliminated);
		}

		void parse_init() override {
			decltype(parse_stack){}.swap(parse_stack); // clear stack

			parse_stack.push_back(terminal_type::Type::eof);
			parse_stack.push_back(this->grammar.get_start_symbol());
		}
		
		std::set<terminal_type> currently_expected_terminals() const {
			if (parse_stack.empty()) {
				return { {terminal_type::Type::eof} };
			}

			if (const auto* terminal = std::get_if<terminal_type>(&parse_stack.back())) {
				return { *terminal };
			} else {
				const auto& nonterminal = std::get<nonterminal_type>(parse_stack.back());
				std::set<terminal_type> result;

				typename grammar_type::prod_type stack_beg;

				auto iter = parse_stack.rbegin();
				do {
					stack_beg.push_back(*iter);
					iter++;
				} while (iter != parse_stack.rend() && !std::holds_alternative<terminal_type>(*std::prev(iter)));

				for (const auto& terminal : this->grammar.first_star(stack_beg)) {
					result.insert(std::get<terminal_type>(terminal));
				}

				return result;
			}
		}

		std::set<terminal_type> sync_set() const {
			if (const auto* nonterminal = std::get_if<nonterminal_type>(&parse_stack.back())) {
				std::set<terminal_type> result;
				for (const auto& symbol : this->grammar.get_follows().at(*nonterminal)) {
					result.insert(symbol);
				}
				return result;
			} else {
				return { std::get<terminal_type>(parse_stack.back()) };
			}
		}
	};
}