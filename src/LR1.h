#pragma once

#include <string_view>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <variant>
#include <list>
#include <stack>

#include "Token.h"
#include "Grammar.h"

namespace dpl {
	class LR1Automaton {
	public:

		struct Configuration {
			std::pair<std::string_view, int> production;
			int pos = 0;
			Token lookahead;

			friend bool operator==(const Configuration lhs, const Configuration& rhs) {
				return (lhs.production == rhs.production) && (lhs.pos == rhs.pos) && (lhs.lookahead == rhs.lookahead);
			}

			auto dot(Grammar& g) const {
				return g[production.first][production.second][pos];
			}

			Configuration next() const {
				return Configuration{ production, pos + 1, lookahead };
			}

			bool atEnd(Grammar& g) const {
				return g[production.first][production.second].size() == pos;
			}
		};

		struct State : private std::vector<Configuration> {

			bool contains(const Configuration& cfg) const {
				for (const Configuration& config : *this) {
					if (config == cfg) return true;
				}
				return false;
			}

			void computeClosure(Grammar& g) {
				size_t old_size;
				do {
					old_size = size();

					for (int i = 0; i < size(); i++) {
						Configuration& config = (*this)[i];
						const ProductionRule& rule = g[config.production.first][config.production.second];

						if (config.pos != rule.size()) {
							if (const auto* nonterminal = std::get_if<std::string_view>(&rule[config.pos])) {

								for (int j = 0; j < g[*nonterminal].size(); j++) {

									config = (*this)[i];

									ProductionRule rule_for_first(std::next(rule.begin(), config.pos + 1), rule.end());
									rule_for_first.push_back(config.lookahead);
									auto first_star_set = g.first_star(rule_for_first.begin(), rule_for_first.end());

									for (const auto terminal : first_star_set) {
										if (!std::holds_alternative<std::monostate>(terminal)) {
											const Configuration new_config = { { *nonterminal, j }, 0, std::get<Token>(terminal) };
											if (!contains(new_config)) {
												emplace_back(new_config);
											}
										}
									}

									first_star_set = std::unordered_set<std::variant<std::monostate, Token>>();

								}

							}
						}
					}

				} while (size() != old_size);
			}

			State successor(std::variant<Token, std::string_view> symbol, Grammar& g) const {
				State result;

				for (const Configuration& config : *this) {
					if (!config.atEnd(g) && config.dot(g) == symbol) {
						result.push_back(config.next());
					}
				}

				result.computeClosure(g);
				return result;
			}

			using std::vector<Configuration>::vector;
			using std::vector<Configuration>::size;
			using std::vector<Configuration>::operator[];
			using std::vector<Configuration>::begin;
			using std::vector<Configuration>::end;
			using std::vector<Configuration>::push_back;
			using std::vector<Configuration>::empty;

		};

		int contains(const State& other) {
			for (int i = 0; i < states.size(); i++) {
				if (states[i].first.size() != other.size()) continue;

				if (std::all_of(states[i].first.begin(), states[i].first.end(), [&other](const Configuration& config) {
					return other.contains(config);
				})) return i;
			}
			return -1;
		}

	public:

		std::vector<std::pair<State, std::map<std::variant<std::monostate, Token, std::string_view>, int>>> states;

		LR1Automaton(Grammar& g) {
			// augment grammar
			std::string augmented_start_symbol{ g.start_symbol };
			std::string_view old_start_symbol = g[g.start_symbol].name;
			do {
				augmented_start_symbol.push_back('_');
			} while (g.contains(augmented_start_symbol));

			std::swap(g.start_symbol, augmented_start_symbol);
			g[g.start_symbol] = { g.start_symbol, {{ old_start_symbol }} };

			// we rely on first sets, so calculate them
			g.initialize();

			// construct initial state
			State start_state;
			start_state.push_back(Configuration{ {g.start_symbol, 0}, 0, Token::Type::EndOfFile });
			states.push_back({ start_state, {} });
			states.back().first.computeClosure(g);

			// add all states and transitions based on alg:
			// #TASK : detect errors (shift/shift and shift/reduce) at automaton-construction
			size_t old_size = 0;
			do {
				old_size = states.size();

				// #TASK : iterate more efficiently through stack (linaer time instead of polynomial!), probably by keeping a chore queue
				for (int i = 0; i < states.size(); i++) {
					auto& [state, transes] = states[i];
					for (size_t j = 0; j < state.size(); j++) {
						const Configuration& config = state[j];

						if (config.atEnd(g)) continue;

						const auto& symbol = config.dot(g);
						State succ = state.successor(symbol, g);

						if (!succ.empty()) {
							int dest = contains(succ);

							std::visit([&](const auto& sym) {
								//transes[sym] = dest == -1 ? states.size() : dest;
								transes.insert_or_assign(sym, dest == -1 ? states.size() : dest);
							}, symbol);

							if (dest == -1) {
								states.push_back({ succ, {} });
								dest = states.size() - 1;
							}
						}
					}
				}
			} while (states.size() != old_size);
		}

	};



	class LR1 {
	public:

		using out_type = std::variant<Token, std::pair<std::string_view, int>>;
		using symbol_type = std::variant<std::monostate, Token, std::string_view>;

		LR1(Grammar& g, ParseTree& pt, Tokenizer& inp) : input(inp), grammar(g), out_tree(pt), tree_builder(grammar) {
			LR1Automaton automaton{ g };

			// generate parse tables
			for (int i = 0; i < automaton.states.size(); i++) {
				auto& [state, transes] = automaton.states[i];

				for (const auto& config : state) {
					if (config.production.first == g.start_symbol
							&& config.production.second == 0
							&& config.pos == 1
							&& config.lookahead == Token::Type::EndOfFile)
					{
						if (hasActionEntry(i, config.lookahead)) std::cout << "Error: Duplicate Action Entries -- Non-LR(1) Grammar!\n";
						else action_table[i][config.lookahead] = std::monostate();

						initial_state = i;
					} else if (config.atEnd(g)) {
						if (hasActionEntry(i, config.lookahead)) std::cout << "Error: Duplicate Action Entries -- Non-LR(1) Grammar!\n";
						else action_table[i][config.lookahead] = config.production;
					}
				}

				for (const auto& [symbol, dest] : transes) {
					if (hasGotoEntry(i, symbol)) std::cout << "Error: Duplicate Action Entries -- Non-LR(1) Grammar!\n";
					else goto_table[i][symbol] = dest;
				}
				
			}


			parse_stack.push(0);
		}

		void operator<<(const Token& t) {
			Token t_ = getTerminalType(t);

			bool terminal_eliminated = false;
			do {

				if (!hasActionEntry(parse_stack.top(), t_)) { // not contains means the action is shift
					if (!hasGotoEntry(parse_stack.top(), t_)) {
						std::cerr << "Syntax error: unexptected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
						return;
					}

					int new_state = goto_table[parse_stack.top()][t_];
					parse_stack.push(new_state);

					terminal_eliminated = true;

					std::cout << "Shift: " << t.stringify() << ", goto " << new_state << '\n';
					tree_builder.addSubTree(t);
				} else if (const auto* prod = std::get_if<std::pair<std::string_view, int>>(&action_table[parse_stack.top()][t_])) { // reduce action
					const ProductionRule& rule = grammar[prod->first][prod->second];

					for (int i = 0; i < rule.size(); i++) parse_stack.pop();

					if (!hasGotoEntry(parse_stack.top(), prod->first)) {
						std::cerr << "Syntax error: unexptected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
						return;
					}

					int new_state = goto_table[parse_stack.top()][prod->first];
					parse_stack.push(new_state);

					std::cout << "Reduce: " << rule << ", goto " << new_state << '\n';
					tree_builder.packTree(*prod, rule.size());
				} else if (std::holds_alternative<std::monostate>(action_table[parse_stack.top()][t_])) { // accept
					terminal_eliminated = true;
					tree_builder.assignToTree(out_tree);
				} else { // report error
					std::cerr << "Syntax error: unexpected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
				}

			} while (!terminal_eliminated);
		}

	private:

		bool hasGotoEntry(const int state, const typename symbol_type& t) {
			return goto_table.contains(state) && goto_table[state].contains(t);
		}

		bool hasActionEntry(const int state, const Token& t) {
			return action_table.contains(state) && action_table[state].contains(t);
		}


		Tokenizer& input;
		Grammar& grammar;
		BottomUpTreeBuilder tree_builder;
		ParseTree& out_tree;

		using action_type = std::variant<std::monostate, int, std::pair<std::string_view, int>>;

		std::unordered_map<int, std::map<symbol_type, int>> goto_table;
		std::unordered_map<int, std::unordered_map<Token, action_type>> action_table;

		int initial_state = -1;
		std::stack<int> parse_stack;

	};
}