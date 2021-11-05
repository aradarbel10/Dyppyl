#pragma once

#include "../tokenizer/Token.h"
#include "../Grammar.h"
#include "../ParseTree.h"
#include "../tokenizer/Tokenizer.h"
#include "Parser.h"

#include <stack>

namespace dpl {
	template<class AutomatonT>
	class LR : public Parser {
	public:

		using state_type = int;
		using accept_action = std::monostate;
		using terminal_type = Terminal;
		using nonterminal_type = std::string_view;

		using out_type = std::variant<Token, RuleRef>;
		using symbol_type = std::variant<std::monostate, terminal_type, nonterminal_type>;

		using action_type = std::variant<accept_action, state_type, RuleRef>;

		LR(Grammar& g) : Parser(g), tb(g) {
			AutomatonT automaton{ grammar };

			// generate parse tables
			for (int i = 0; i < automaton.states.size(); i++) {
				auto& [state, transes] = automaton.states[i];

				auto row = state.getActions(g);
				for (const auto& [token, action] : row) {
					// #TASK : get these error messages to work
					if (!addActionEntry(i, token, action)) std::cerr << "Error: Duplicate Action Entries -- Non-" << /*getParserName<decltype(*this)> <<*/ " Grammar!\n";
				}

				for (const auto& [symbol, dest] : transes) {
					if (!addGotoEntry(i, symbol, dest)) std::cerr << "Error: Duplicate Goto Entries -- Non-" << /*getParserName<decltype(*this)> <<*/ " Grammar!\n";
				}
			}
		}
		
		void parse_init() override {
			std::stack<state_type>{}.swap(parse_stack); // clear stack
			parse_stack.push(0);
		}

		void operator<<(const Token& t) {
			terminal_type t_ = t;

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
					tree_builder().pushNode(t);
				} else if (const auto* prod = std::get_if<RuleRef>(&getActionEntry(parse_stack.top(), t_))) { // reduce action
					const ProductionRule& rule = prod->getRule();

					for (int i = 0; i < rule.size(); i++) parse_stack.pop();

					if (!hasGotoEntry(parse_stack.top(), prod->name)) {
						std::cerr << "Syntax error: unexptected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
						return;
					}

					state_type new_state = getGotoEntry(parse_stack.top(), prod->name);
					parse_stack.push(new_state);

					std::cout << "Reduce: " << rule << ", goto " << new_state << '\n';
					tree_builder().pushNode(*prod);
				} else if (std::holds_alternative<std::monostate>(getActionEntry(parse_stack.top(), t_))) { // accept
					terminal_eliminated = true;

					this->tree_builder().assignToTree(out_tree);

				} else { // report error
					std::cerr << "Syntax error: unexpected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
				}

			} while (!terminal_eliminated);
		}


	protected:

		bool hasGotoEntry(const int state, const typename symbol_type& t) {
			return goto_table.contains(state) && goto_table[state].contains(t);
		}

		bool hasActionEntry(const int state, const terminal_type& t) {
			return action_table.contains(state) && action_table[state].contains(t);
		}

		state_type& getGotoEntry(const int state, const typename symbol_type& t) {
			return goto_table[state][t];
		}

		action_type& getActionEntry(const int state, const terminal_type& t) {
			return action_table[state][t];
		}

		bool addGotoEntry(const int state, const symbol_type& t, state_type dest) {
			state_type init_size = state_type(goto_table[state].size());
			goto_table[state][t] = dest;
			return init_size != goto_table[state].size();
		}

		bool addActionEntry(const int state, const terminal_type& t, action_type dest) {
			state_type init_size = state_type(action_table[state].size());
			action_table[state][t] = dest;
			return init_size != action_table[state].size();
		}

	protected:

		BottomUpTreeBuilder tb;
		TreeBuilder& tree_builder() {
			return tb;
		}

		std::unordered_map<state_type, std::map<symbol_type, state_type>> goto_table;
		std::unordered_map<state_type, std::map<terminal_type, action_type>> action_table;

		state_type initial_state = -1;
		std::stack<state_type> parse_stack;

	};

	struct Configuration {
		RuleRef production;
		int pos = 0;

		Configuration(RuleRef prod_, int pos_)
			: production(prod_), pos(pos_) { }

		bool operator==(const Configuration&) const = default;

		auto dot(Grammar& g) const {
			return production.getRule()[pos];
		}

		void next() {
			pos++;
		}

		bool atEnd(Grammar& g) const {
			return production.getRule().size() == pos;
		}

		static auto getStartConfig(Grammar& g) {
			return Configuration{ {g, g.start_symbol, 0}, 0 };
		}

		auto toEnd(Grammar& g) {
			auto result = *this;
			result.pos = int(production.getRule().size());
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
				result.push_back({ { g, *nonterminal, i }, 0 });
			}

			return result;
		} else return {};

		
	}

	template<class ConfigT>
	struct State : protected std::vector<ConfigT> {

		using config_type = ConfigT;

		using terminal_type = Terminal;
		using nonterminal_type = std::string_view;

		void computeClosure(Grammar& g) {
			size_t old_size;
			do {
				old_size = size();

				// iterate through all configurations in state
				for (int i = 0; i < size(); i++) {
					const config_type& config = (*this)[i];
					const ProductionRule& rule = config.production.getRule();

					const auto new_configs = computeConfigClosure(g, config);
					
					for (const config_type& new_config : new_configs) {
						if (!contains(new_config)) {
							push_back(new_config);
						}
					}
				}

			} while (size() != old_size);
		}

		bool contains(const config_type& cfg) const {
			for (const config_type& config : *this) {
				if (config == cfg) return true;
			}
			return false;
		}

		State successor(std::variant<terminal_type, nonterminal_type> symbol, Grammar& g) const {
			State result;

			for (const config_type& config : *this) {
				if (config.atEnd(g)) continue;

				if (config.dot(g) == symbol) {
					config_type next_config = config;
					next_config.next();
					result.push_back(next_config);
				}
			}

			result.computeClosure(g);
			return result;
		}

		virtual std::map<typename LR<void>::terminal_type, typename LR<void>::action_type> getActions(Grammar& g) const {
			std::map<typename LR<void>::terminal_type, typename LR<void>::action_type> result;

			for (const config_type& config : *this) {
				if (config == config_type::getStartConfig(g).toEnd(g))
					result[Terminal::Type::Unknown] = std::monostate{};
				else if (config.atEnd(g))
					result[Terminal::Type::Unknown] = config.production;
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

				if (std::all_of(states[i].first.begin(), states[i].first.end(), [&other](const StateT::config_type& config) {
					return other.contains(config);
				})) return i;
			}
			return -1;
		}

	public:
		
		using terminal_type = Terminal;
		using nonterminal_type = std::string_view;
		using state_type = StateT;
		using symbol_type = std::variant<std::monostate, terminal_type, nonterminal_type>;
		using whole_state_type = std::pair<state_type, std::map<symbol_type, int>>;

		std::vector<whole_state_type> states;

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
			start_state.push_back(StateT::config_type::getStartConfig(g));
			states.push_back({ start_state, {} });
			states.back().first.computeClosure(g);

			// add all states and transitions based on alg:
			// #TASK : detect errors (shift/shift and shift/reduce) at automaton-construction
			std::queue<int> chore_queue;
			chore_queue.push(0);

			size_t old_size = 0;
			do {
				old_size = states.size();

				// #TASK : iterate more efficiently through stack (linaer time instead of polynomial!), probably by keeping a chore queue
				while(!chore_queue.empty()) { int i = chore_queue.front();

					for (size_t j = 0; j < states[i].first.size(); j++) {
						auto& [state, transes] = states[i];
						const StateT::config_type& config = states[i].first[j];

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
								chore_queue.push(int(states.size()) - 1);
							}
						}
					}

					chore_queue.pop();
				}
			} while (states.size() != old_size);
		}

	};


	typedef LR<LR0Automaton<State<Configuration>>> LR0;

	template<>
	const char* getParserName<LR0> = "LR(0)";
}