#pragma once

#include "../Token.h"
#include "../Grammar.h"
#include "../ParseTree.h"
#include "../Tokenizer.h"

#include <stack>

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

					// iterate through all configurations in state
					for (int i = 0; i < size(); i++) {
						const Configuration& config = (*this)[i];
						const ProductionRule& rule = g[config.production.first][config.production.second];

						if (!config.atEnd(g)) {
							if (const auto* nonterminal = std::get_if<std::string_view>(&rule[config.pos])) {

								// iterate through all productions in nonterminal definition
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

			State successor(std::variant<Token, std::string_view> symbol, Grammar& g) const {
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

			g.initialize();

			// construct initial state
			State start_state;
			start_state.push_back(Configuration{ {g.start_symbol, 0}, 0 });
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




	class LR {
	public:

		using state_type = int;
		using accept_action = std::monostate;

		using out_type = std::variant<Token, std::pair<std::string_view, int>>;
		using symbol_type = std::variant<std::monostate, Token, std::string_view>;

		using action_type = std::variant<accept_action, state_type, std::pair<std::string_view, int>>;

		LR(Grammar& g, ParseTree& pt, Tokenizer& inp);
		void operator<<(const Token& t);


	protected:

		bool hasGotoEntry(const int state, const typename symbol_type& t) {
			return goto_table.contains(state) && goto_table[state].contains(t);
		}

		bool hasActionEntry(const int state, const Token& t) {
			return action_table.contains(state);
		}

		action_type& getActionEntry(const int state, const Token& t) {
			return action_table[state][Token::Type::Unknown];
		}


	protected:

		Tokenizer& input;
		Grammar& grammar;
		BottomUpTreeBuilder tree_builder;
		ParseTree& out_tree;

		std::unordered_map<int, std::map<symbol_type, int>> goto_table;
		std::unordered_map<int, std::map<Token, action_type>> action_table;

		int initial_state = -1;
		std::stack<int> parse_stack;

	};
}