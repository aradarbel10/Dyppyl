#pragma once

#include <string_view>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <variant>

#include "Token.h"
#include "Grammar.h"

namespace dpl {

	template<typename KwdT, typename SymT>
	class LR0Automaton {
	public:

		using Token = Token<KwdT, SymT>;
		using ProductionRule = ProductionRule<KwdT, SymT>;
		using Grammar = Grammar<KwdT, SymT>;

	private:

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

		std::vector<std::pair<State, std::map<std::variant<std::monostate, Token, std::string_view>, size_t>>> states;

		LR0Automaton(Grammar& g) {
			// augment grammar
			std::string augmented_start_symbol{ g.start_symbol };
			do {
				augmented_start_symbol.push_back('_');
			} while (g.contains(augmented_start_symbol));

			g[augmented_start_symbol] = { augmented_start_symbol, {{ g.start_symbol }} };

			// construct initial state
			State start_state;
			start_state.push_back(Configuration{{augmented_start_symbol, 0}, 0});
			states.push_back({ start_state, {} });
			states.back().first.computeClosure(g);

			// add all states and transitions based on alg:
			size_t old_size;
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


	template<typename KwdT, typename SymT>
	class LR0 {
	public:

		using Token = Token<KwdT, SymT>;
		using ProductionRule = ProductionRule<KwdT, SymT>;
		using Nonterminal = Nonterminal<KwdT, SymT>;
		using Grammar = Grammar<KwdT, SymT>;
		using ParseTree = ParseTree<KwdT, SymT>;
		using out_type = std::variant<Token, std::pair<std::string_view, int>>;

		LR0(Grammar& g, std::string_view s, ParseTree& pt) : grammar(g), start_symbol(s), out_tree(pt) {
			// generate parse tables
			//	- action table
			//	- transition table
		}

		void fetchNext() {

		}

		void generateParseTables() {
			
		}

	private:

		std::string_view start_symbol;
		Grammar& grammar;
		ParseTree& out_tree;

	};
}