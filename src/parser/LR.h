#pragma once

#include "../Token.h"
#include "../Grammar.h"
#include "../ParseTree.h"
#include "../Tokenizer.h"

#include <stack>

namespace dpl {


	template<class AutomatonT>
	class LR {
	public:

		using state_type = int;
		using accept_action = std::monostate;

		using out_type = std::variant<Token, std::pair<std::string_view, int>>;
		using symbol_type = std::variant<std::monostate, Token, std::string_view>;

		using action_type = std::variant<accept_action, state_type, std::pair<std::string_view, int>>;

		LR(Grammar& g, ParseTree& pt, Tokenizer& inp) : input(inp), grammar(g), out_tree(pt), tree_builder(grammar) {
			AutomatonT automaton{ grammar };

			// generate parse tables
			for (int i = 0; i < automaton.states.size(); i++) {
				auto& [state, transes] = automaton.states[i];

				auto row = state.getActions(g);
				for (const auto& [token, action] : row) {
					if (!addActionEntry(i, token, action)) std::cerr << "Error: Duplicate Action Entries -- Non-LR(1) Grammar!\n";
				}

				for (const auto& [symbol, dest] : transes) {
					if (!addGotoEntry(i, symbol, dest)) std::cerr << "Error: Duplicate Goto Entries -- Non-LR(1) Grammar!\n";
				}
			}


			parse_stack.push(0);
		}
		
		void operator<<(const Token& t) {
			Token t_ = getTerminalType(t);

			bool terminal_eliminated = false;
			do {

				// #TASK : make sure stack is non-empty
				if (!hasActionEntry(parse_stack.top(), t_)) { // not contains means the action is shift
					if (!hasGotoEntry(parse_stack.top(), t_)) {
						std::cerr << "Syntax error: unexptected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
						return;
					}

					state_type new_state = getGotoEntry(parse_stack.top(), t_);
					parse_stack.push(new_state);

					terminal_eliminated = true;

					std::cout << "Shift: " << t.stringify() << ", goto " << new_state << '\n';
					tree_builder.addSubTree(t);
				} else if (const auto* prod = std::get_if<std::pair<std::string_view, int>>(&getActionEntry(parse_stack.top(), t_))) { // reduce action
					const ProductionRule& rule = grammar[(*prod).first][(*prod).second];

					for (int i = 0; i < rule.size(); i++) parse_stack.pop();

					if (!hasGotoEntry(parse_stack.top(), (*prod).first)) {
						std::cerr << "Syntax error: unexptected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
						return;
					}

					state_type new_state = getGotoEntry(parse_stack.top(), prod->first);
					parse_stack.push(new_state);

					std::cout << "Reduce: " << rule << ", goto " << new_state << '\n';
					tree_builder.packTree(*prod, rule.size());
				} else if (std::holds_alternative<std::monostate>(getActionEntry(parse_stack.top(), t_))) { // accept
					terminal_eliminated = true;
					tree_builder.assignToTree(out_tree);
				} else { // report error
					std::cerr << "Syntax error: unexpected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
				}

			} while (!terminal_eliminated);
		}


	protected:

		bool hasGotoEntry(const int state, const typename symbol_type& t) {
			return goto_table.contains(state) && goto_table[state].contains(t);
		}

		bool hasActionEntry(const int state, const Token& t) {
			return action_table.contains(state) && action_table[state].contains(t);
		}

		state_type& getGotoEntry(const int state, const typename symbol_type& t) {
			return goto_table[state][t];
		}

		action_type& getActionEntry(const int state, const Token& t) {
			return action_table[state][t];
		}

		bool addGotoEntry(const int state, const symbol_type& t, state_type dest) {
			size_t init_size = goto_table[state].size();
			goto_table[state][t] = dest;
			return init_size != goto_table[state].size();
		}

		bool addActionEntry(const int state, const Token& t, action_type dest) {
			size_t init_size = action_table[state].size();
			action_table[state][t] = dest;
			return init_size != action_table[state].size();
		}

	protected:

		Tokenizer& input;
		Grammar& grammar;
		BottomUpTreeBuilder tree_builder;
		ParseTree& out_tree;

		std::unordered_map<state_type, std::map<symbol_type, state_type>> goto_table;
		std::unordered_map<state_type, std::map<Token, action_type>> action_table;

		state_type initial_state = -1;
		std::stack<state_type> parse_stack;

	};

	struct Configuration {
		std::pair<std::string_view, int> production;
		int pos = 0;

		Configuration(std::pair<std::string_view, int> prod_, int pos_)
			: production(prod_), pos(pos_) { }

		bool operator==(const Configuration&) const = default;

		auto dot(Grammar& g) const {
			return g[production.first][production.second][pos];
		}

		void next() {
			pos++;
		}

		bool atEnd(Grammar& g) const {
			return g[production.first][production.second].size() == pos;
		}

		static auto getStartConfig(Grammar& g) {
			return Configuration{ {g.start_symbol, 0}, 0 };
		}

		auto toEnd(Grammar& g) {
			auto result = *this;
			result.pos = g[production.first][production.second].size();
			return result;
		}

	};

	template<class ConfigT>
	std::vector<ConfigT> computeConfigClosure(Grammar& g, const ConfigT& config) = delete;

	template<>
	std::vector<Configuration> computeConfigClosure<Configuration>(Grammar& g, const Configuration& config) {
		if (config.atEnd(g)) return {};

		auto symbol = config.dot(g);
		if (const auto* nonterminal = std::get_if<std::string_view>(&symbol)) {

			std::vector<Configuration> result;

			size_t prods_amount = g[*nonterminal].size();
			result.reserve(prods_amount);
			for (int i = 0; i < prods_amount; i++) {
				result.push_back({ { *nonterminal, i }, 0 });
			}

			return result;
		} else return {};

		
	}

	template<class ConfigT>
	struct State : protected std::vector<ConfigT> {

		using config_t = ConfigT;

		void computeClosure(Grammar& g) {
			size_t old_size;
			do {
				old_size = size();

				// iterate through all configurations in state
				for (int i = 0; i < size(); i++) {
					const ConfigT& config = (*this)[i];
					const ProductionRule& rule = g[config.production.first][config.production.second];

					const auto new_configs = computeConfigClosure(g, config);
					
					for (const ConfigT& new_config : new_configs) {
						if (!contains(new_config)) {
							push_back(new_config);
						}
					}
				}

			} while (size() != old_size);
		}

		bool contains(const ConfigT& cfg) const {
			for (const ConfigT& config : *this) {
				if (config == cfg) return true;
			}
			return false;
		}

		State successor(std::variant<Token, std::string_view> symbol, Grammar& g) const {
			State result;

			for (const ConfigT& config : *this) {
				if (config.atEnd(g)) continue;

				if (config.dot(g) == symbol) {
					ConfigT next_config = config;
					next_config.next();
					result.push_back(next_config);
				}
			}

			result.computeClosure(g);
			return result;
		}

		virtual std::map<Token, typename LR<void>::action_type> getActions(Grammar& g) const {
			std::map<Token, typename LR<void>::action_type> result;

			for (const ConfigT& config : *this) {
				if (config == ConfigT::getStartConfig(g).toEnd(g))
					result[Token::Type::Unknown] = std::monostate();
				else if (config.atEnd(g))
					result[Token::Type::Unknown] = config.production;
			}

			return result;
		}

		using std::vector<ConfigT>::vector;
		using std::vector<ConfigT>::size;
		using std::vector<ConfigT>::operator[];
		using std::vector<ConfigT>::begin;
		using std::vector<ConfigT>::end;
		using std::vector<ConfigT>::push_back;
		using std::vector<ConfigT>::empty;

	};

	template<class StateT>
	class LR0Automaton {
	public:

		int contains(const StateT& other) const {
			for (int i = 0; i < states.size(); i++) {
				if (states[i].first.size() != other.size()) continue;

				if (std::all_of(states[i].first.begin(), states[i].first.end(), [&other](const StateT::config_t& config) {
					return other.contains(config);
				})) return i;
			}
			return -1;
		}

	public:

		std::vector<std::pair<StateT, std::map<std::variant<std::monostate, Token, std::string_view>, int>>> states;

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
			StateT start_state;
			start_state.push_back(StateT::config_t::getStartConfig(g));
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
						const StateT::config_t& config = state[j];

						if (config.atEnd(g)) continue;

						const auto& symbol = config.dot(g);
						StateT succ = state.successor(symbol, g);

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


	typedef LR<LR0Automaton<State<Configuration>>> LR0;
}