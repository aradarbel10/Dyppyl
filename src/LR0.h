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

	class LR0Automaton {
	public:

		struct Configuration {
			std::pair<std::string_view, int> production;
			int pos = 0;

			friend bool operator==(const Configuration lhs, const Configuration& rhs) {
				return (lhs.production == rhs.production) && (lhs.pos == rhs.pos);
			}

			auto dot(Grammar& g) const {
				return g[production.first][production.second][pos];
			}

			Configuration next() const {
				return Configuration{ production, pos + 1 };
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
						const Configuration& config = (*this)[i];
						const ProductionRule& rule = g[config.production.first][config.production.second];

						if (config.pos != rule.size()) {
							if (const auto* nonterminal = std::get_if<std::string_view>(&rule[config.pos])) {

								for (int j = 0; j < g[*nonterminal].size(); j++) {
									const Configuration new_config = { { *nonterminal, j }, 0 };
									if (!contains(new_config)) {
										push_back(new_config);
									}
								}

							}
						}
					}

				} while (size() != old_size);
			}

			State successor(std::variant<std::monostate, Token, std::string_view> symbol, Grammar& g) const {
				State result;

				for (const Configuration& config : *this) {
					if (config.dot(g) == symbol) {
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

		int getState(const Configuration& config) {
			for (int i = 0; i < states.size(); i++) {
				if (std::any_of(states[i].first.begin(), states[i].first.end(), [&config](const Configuration& cfg) {
					if (config == cfg) return true;
					return false;
				})) return i;
			}
			return -1;
		}

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

		LR0Automaton(Grammar& g) {
			// augment grammar
			std::string augmented_start_symbol{ g.start_symbol };
			std::string_view old_start_symbol = g[g.start_symbol].name;
			do {
				augmented_start_symbol.push_back('_');
			} while (g.contains(augmented_start_symbol));

			std::swap(g.start_symbol, augmented_start_symbol);
			g[g.start_symbol] = { g.start_symbol, {{ old_start_symbol }} };
			

			// construct initial state
			State start_state;
			start_state.push_back(Configuration{{g.start_symbol, 0}, 0});
			states.push_back({ start_state, {} });
			states.back().first.computeClosure(g);

			// add all states and transitions based on alg:
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
								transes[sym] = dest == -1 ? states.size() : dest;
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


	class LR0 {
	public:

		using out_type = std::variant<Token, std::pair<std::string_view, int>>;
		using symbol_type = std::variant<std::monostate, Token, std::string_view>;

		LR0(Grammar& g, ParseTree& pt, Tokenizer& inp) : input(inp), grammar(g), out_tree(pt), tree_builder(grammar) {
			LR0Automaton automaton{ g };

			// generate parse tables
			for (int i = 0; i < automaton.states.size(); i++) {
				auto& [state, transes] = automaton.states[i];

				if (transes.empty()) {
					assert(state.size() == 1 && state[0].atEnd(g));

					if (state[0].production.first == g.start_symbol) {
						action_table[i] = std::monostate{};
						initial_state = i;
					} else {
						action_table[i] = state[0].production;
					}
					
				} else {
					goto_table[i] = transes;
				}
			}
			

			parse_stack.push(0);
		}

		void operator<<(const Token& t) {
			bool terminal_eliminated = false;
			do {
				
				if (!action_table.contains(parse_stack.top())) { // not contains means the action is shift
					if (!hasGotoEntry(parse_stack.top(), t)) {
						std::cerr << "Syntax error: unexptected token " << t << "at position (" << dpl::log::streamer{ t.pos } << ")\n";
						return;
					}

					int new_state = goto_table[parse_stack.top()][t];
					parse_stack.push(new_state);

					terminal_eliminated = true;

					tree_builder.addSubTree(t);
				} else if (const auto* prod = std::get_if<std::pair<std::string_view, int>>(&action_table[parse_stack.top()])) { // reduce action
					const ProductionRule& rule = grammar[(*prod).first][(*prod).second];

					for (int i = 0; i < rule.size(); i++) parse_stack.pop();

					if (!hasGotoEntry(parse_stack.top(), (*prod).first)) {
						std::cerr << "Syntax error: unexptected token " << t << "at position (" << dpl::log::streamer{ t.pos } << ")\n";
						return;
					}

					int new_state = goto_table[parse_stack.top()][(*prod).first];
					parse_stack.push(new_state);

					tree_builder.packTree(*prod);
				} else if (std::holds_alternative<std::monostate>(action_table[parse_stack.top()])) { // accept
					terminal_eliminated = true;

					tree_builder.assignToTree(out_tree);
				} else { // report error
					std::cerr << "Syntax error: unexpected token " << t << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
				}

			} while (!terminal_eliminated);
		}

	private:

		bool hasGotoEntry(const int state, const typename symbol_type& t) {
			return goto_table.contains(state) && goto_table[state].contains(t);
		}


		Tokenizer& input;
		Grammar& grammar;
		BottomUpTreeBuilder tree_builder;
		ParseTree& out_tree;

		std::unordered_map<int, std::map<symbol_type, int>> goto_table;
		std::unordered_map<int, std::variant<std::monostate, std::pair<std::string_view, int>>> action_table;

		int initial_state = -1;
		std::stack<int> parse_stack;

	};
}