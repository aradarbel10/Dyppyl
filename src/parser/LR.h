#pragma once

#include "../tokenizer/Token.h"
#include "../Grammar.h"
#include "../ParseTree.h"
#include "../tokenizer/Tokenizer.h"
#include "Parser.h"

#include <stack>
#include <format>

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

		LR(Grammar& g, Options ops = {}) : Parser(g, ops), tb(g) {
			AutomatonT automaton{ grammar };

			if (options.log_automaton) {
				options.logprint("Parser Automaton", automaton);
			}
			options.flush_logs();

			// generate parse tables
			for (int i = 0; i < automaton.states.size(); i++) {
				auto& [state, transes] = automaton.states[i];

				auto row = state.getActions(g);
				for (const auto& [token, action] : row) {
					if (!addActionEntry(i, token, action) && options.log_errors)
						options.logprintln("Errors", "grammar error: Duplicate Action Entries -- Non-LL0 Grammar!");
				}

				for (const auto& [symbol, dest] : transes) {
					// #TASK : get these error messages to work
					if (!addGotoEntry(i, symbol, dest) && options.log_errors)
						options.logprintln("Errors", "grammar error: Duplicate Goto Entries -- Non-LL0 Grammar!");
				}
			}

			if (options.log_parse_table) {
				options.logprint("Parse Table", "Action Table:\n");
				for (const auto& [state, row] : action_table) {
					options.logprint("Parse Table", state, ": ");
					for (const auto& [terminal, action] : row) {
						options.logprint("Parse Table", terminal, "(");

						if (std::holds_alternative<std::monostate>(action))
							options.logprint("Parse Table", "acc");
						else if (auto* rule = std::get_if<dpl::RuleRef>(&action))
							options.logprint("Parse Table", "reduce ", rule->name, rule->prod);
						else
							options.logprint("Parse Table", "shift ", std::get<state_type>(action));

						options.logprint("Parse Table", ")  ");
					}
					options.logprint("Parse Table", "\n");
				}

				options.logprint("Parse Table", "Goto Table:\n");
				for (const auto& [state, row] : goto_table) {
					options.logprint("Parse Table", state, ": ");
					for (const auto& [symbol, newstate] : row) {
						options.logprint("Parse Table", symbol, "(", newstate, ")  ");
					}
					options.logprint("Parse Table", "\n");
				}
			}
		}
		
		void parse_init() override {
			std::stack<state_type>{}.swap(parse_stack); // clear stack
			parse_stack.push(0);
		}

		void operator<<(const Token& t_) {
			terminal_type t = t_;

			//if (options.error_mode == Options::ErrorMode::RecoverOnFollow && !fixed_last_error) {
			//	if (sync_set().contains(t)) {
			//		parse_stack.pop();
			//		fixed_last_error = true;
			//	} else {
			//		return;
			//	}
			//}

			bool terminal_eliminated = false;
			do {

				// #TASK : make sure stack is non-empty
				if (!hasActionEntry(parse_stack.top(), t)) { // not contains means the action is shift
					if (!hasGotoEntry(parse_stack.top(), t)) {
						err_unexpected_token(t_);
						return;
					}

					state_type new_state = getGotoEntry(parse_stack.top(), t);
					parse_stack.push(new_state);

					terminal_eliminated = true;

					if (options.log_step_by_step)
						options.logprintln("Parser Trace", "Shift: ", t.stringify(), ", goto ", new_state);

					tree_builder().pushNode(t_);
				} else if (const auto* prod = std::get_if<RuleRef>(&getActionEntry(parse_stack.top(), t))) { // reduce action
					const ProductionRule& rule = prod->getRule();

					for (int i = 0; i < rule.size(); i++) parse_stack.pop();

					if (!hasGotoEntry(parse_stack.top(), prod->name)) {
						err_unexpected_token(t_);
						return;
					}

					state_type new_state = getGotoEntry(parse_stack.top(), prod->name);
					parse_stack.push(new_state);

					if (options.log_step_by_step)
						options.logprintln("Parser Trace", "Reduce: ", rule, ", goto ", new_state);

					tree_builder().pushNode(*prod);
				} else if (std::holds_alternative<std::monostate>(getActionEntry(parse_stack.top(), t_))) { // accept
					terminal_eliminated = true;

					this->tree_builder().assignToTree(out_tree);

				} else { // report error
					err_unexpected_token(t_);
				}

			} while (!terminal_eliminated);
		}

		std::set<terminal_type> currently_expected_terminals() const {
			if (parse_stack.empty()) {
				return {{ dpl::Terminal::Type::EndOfFile }};
			}

			std::set<terminal_type> result;

			if (goto_table.contains(parse_stack.top())) {
				const auto& row = goto_table.at(parse_stack.top());
				for (const auto& [symbol, state] : row) {
					if (const auto* terminal = std::get_if<terminal_type>(&symbol)) {
						result.insert(*terminal);
					}
				}
			}

			if (action_table.contains(parse_stack.top())) {
				const auto& row = action_table.at(parse_stack.top());
				for (const auto& [terminal, action] : row) {
					result.insert(terminal);
				}
			}

			return result;
			
		}

		std::set<terminal_type> sync_set() const {
			return {};
		}

	protected:

		bool hasGotoEntry(state_type state, const typename symbol_type& t) {
			return goto_table.contains(state) && goto_table[state].contains(t);
		}

		bool hasActionEntry(state_type state, const terminal_type& t) {
			return action_table.contains(state) && action_table[state].contains(t);
		}

		state_type& getGotoEntry(state_type state, const typename symbol_type& t) {
			return goto_table[state][t];
		}

		action_type& getActionEntry(state_type state, const terminal_type& t) {
			return action_table[state][t];
		}

		void removeActionEntry(state_type state, const terminal_type& t) {
			if (action_table.contains(state)) {
				action_table[state].erase(t);
				if (action_table[state].empty())
					action_table.erase(state);
			}
		}

		bool addGotoEntry(const int state, const symbol_type& t, state_type dest) {
			// resolve shift-reduce conflicts based on assoc and pred values
			if (std::holds_alternative<terminal_type>(t)) {
				terminal_type terminal = std::get<terminal_type>(t);

				if (hasActionEntry(state, terminal))
				if (const auto* r = std::get_if<dpl::RuleRef>(&getActionEntry(state, std::get<terminal_type>(t)))) {
					const auto& rule = r->getRule();

					if (grammar.terminal_precs[terminal] < rule.prec) {
						return true;
					} else if (grammar.terminal_precs[terminal] == rule.prec) {
						if (rule.assoc == Assoc::Left) {
							return true;
						} else if (rule.assoc == Assoc::None) {
							return false;
						} else {
							removeActionEntry(state, terminal);
						}
					} else {
						removeActionEntry(state, terminal);
					}
				}
			}

			goto_table[state][t] = dest;
			return true;
		}

		bool addActionEntry(const int state, const terminal_type& t, action_type dest) {
			size_t init_size = size_t(action_table[state].size());
			action_table[state][t] = dest;
			return init_size != action_table[state].size();
		}

	protected:

		//std::set<terminal_type> sync_set() const {


		//	if (const auto* nonterminal = std::get_if<nonterminal_type>(&parse_stack.top())) {
		//		std::set<terminal_type> result;
		//		for (const auto& symbol : grammar.follows[*nonterminal]) {
		//			result.insert(symbol);
		//		}
		//		return result;
		//	} else {
		//		return { std::get<terminal_type>(parse_stack.top()) };
		//	}
		//}

		BottomUpTreeBuilder tb;
		TreeBuilder& tree_builder() { return tb; }

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

		void print_lookahead(std::ostream& os) const {}

	};

	template<class ConfigT>
	std::vector<ConfigT> computeConfigClosure(Grammar& g, const ConfigT& config) = delete;

	template<>
	inline std::vector<Configuration> computeConfigClosure<Configuration>(Grammar& g, const Configuration& config) {
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
					result[Terminal::Type::EndOfFile] = std::monostate{};
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


		friend std::ostream& operator<<(std::ostream& os, const State& state) {
			os << "<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\">\n";

			for (auto iter = state.begin(); iter != state.end(); iter++) {
				const State::config_type& config = *iter;

				os << "<TR>\n";

				os << "<TD ALIGN=\"left\"";
				if (iter == state.begin()) os << " WIDTH=\"100\"";
				os << ">";

				os << config.production.name << "  &#8594; "; // right arrow

				const auto& rule = config.production.getRule();
				for (int j = 0; j < rule.size(); j++) {
					if (j == config.pos) {
						os << "<FONT COLOR=\"blue\">&#183;</FONT>";
					}

					os << " " << dpl::streamer{rule[j]} << " ";
				}

				if (config.pos == rule.size()) {
					os << " <FONT COLOR=\"blue\">&#183;</FONT> ";
				}

				os << "</TD>\n";

				config.print_lookahead(os);

				os << "</TR>\n";
			}

			os << "</TABLE>\n";
			return os;
		}

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

		friend std::ostream& operator<<(std::ostream& os, const LR0Automaton& automaton) {
			os << "digraph {\nnode [shape = box];\nstart [shape = none];\n\n";

			for (int i = 0; i < automaton.states.size(); i++) {
				const auto& [state, transes] = automaton.states[i];

				os << "s" << i << "[label = <" << state << ">];\n\n";

				if (i == 0) os << "start -> s0;\n";

				for (const auto& [symbol, dest] : transes) {
					os << "s" << i << " -> s" << dest << " [label = \"" << dpl::streamer{symbol} << "\"];\n";
				}
			}

			os << "}";

			return os;
		}

	};


	typedef LR<LR0Automaton<State<Configuration>>> LR0;
}