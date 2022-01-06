#pragma once

#include "../tokenizer/Token.h"
#include "../Grammar.h"
#include "../ParseTree.h"
#include "../tokenizer/Tokenizer.h"
#include "Parser.h"

#include <stack>
#include <format>

namespace dpl {
	template<class AutomatonT, typename GrammarT = dpl::Grammar<>, typename LexiconT = dpl::Lexicon<>>
	class LR : public Parser<GrammarT, LexiconT> {
	public:
		using automaton_type = AutomatonT;
		using grammar_type = GrammarT;
		using lexicon_type = LexiconT;

		using state_type = int;
		using accept_action = std::monostate;
		using terminal_type = grammar_type::terminal_type;
		using token_type = grammar_type::token_type;
		using nonterminal_type = grammar_type::nonterminal_type;

		using out_type = std::variant<token_type, RuleRef<grammar_type>>;
		using symbol_type = std::variant<terminal_type, nonterminal_type>;

		using action_type = std::variant<accept_action, state_type, RuleRef<grammar_type>>;

		LR(grammar_type& g, lexicon_type& l, Parser<grammar_type, lexicon_type>::Options ops = {}) : Parser<grammar_type, lexicon_type>(g, l, ops), tb(g) {
			automaton_type automaton{ this->grammar };

			if (this->options.log_automaton) {
				this->options.logprint("Parser Automaton", automaton);
			}
			this->options.flush_logs();

			// generate parse tables
			for (int i = 0; i < automaton.states.size(); i++) {
				auto& [state, transes] = automaton.states[i];

				auto row = state.getActions(g);
				for (const auto& [token, action] : row) {
					if (!addActionEntry(i, token, action) && this->options.log_errors)
						this->options.logprintln("Errors", "grammar error: Duplicate Action Entries -- Non-LL0 Grammar!");
				}

				for (const auto& [symbol, dest] : transes) {
					// #TASK : get these error messages to work
					if (!addGotoEntry(i, symbol, dest) && this->options.log_errors)
						this->options.logprintln("Errors", "grammar error: Duplicate Goto Entries -- Non-LL0 Grammar!");
				}
			}

			if (this->options.log_parse_table) {
				this->options.logprint("Parse Table", "Action Table:\n");
				for (const auto& [state, row] : action_table) {
					this->options.logprint("Parse Table", state, ": ");
					for (const auto& [terminal, action] : row) {
						this->options.logprint("Parse Table", terminal, "(");

						if (std::holds_alternative<accept_action>(action))
							this->options.logprint("Parse Table", "acc");
						else if (auto* rule = std::get_if<RuleRef<grammar_type>>(&action))
							this->options.logprint("Parse Table", "reduce ", rule->get_name(), rule->get_prod());
						else
							this->options.logprint("Parse Table", "shift ", std::get<state_type>(action));

						this->options.logprint("Parse Table", ")  ");
					}
					this->options.logprint("Parse Table", "\n");
				}

				this->options.logprint("Parse Table", "Goto Table:\n");
				for (const auto& [state, row] : goto_table) {
					this->options.logprint("Parse Table", state, ": ");
					for (const auto& [symbol, newstate] : row) {
						this->options.logprint("Parse Table", symbol, "(", newstate, ")  ");
					}
					this->options.logprint("Parse Table", "\n");
				}
			}
		}
		
		void parse_init() override {
			std::stack<state_type>{}.swap(parse_stack); // clear stack
			parse_stack.push(0);
		}

		void operator<<(const token_type& t_) {
			terminal_type t = t_;

			if (this->options.error_mode == ErrorMode::Panic && !this->fixed_last_error) {
				size_t states_to_pop = 0;
				for (auto state = parse_stack._Get_container().rbegin(); state != parse_stack._Get_container().rend(); state++) {
					if (hasActionEntry(*state, t) || hasGotoEntry(*state, t))
						break;
					else states_to_pop++;
				}

				if (states_to_pop != parse_stack.size()) {
					for (int i = 0; i < states_to_pop; i++) parse_stack.pop();
					this->fixed_last_error = true;
				} else {
					return;
				}
			}

			bool terminal_eliminated = false;
			do {

				// #TASK : make sure stack is non-empty
				if (!hasActionEntry(parse_stack.top(), t)) { // not contains means the action is shift
					if (!hasGotoEntry(parse_stack.top(), t)) {
						this->err_unexpected_token(t_);
						return;
					}

					state_type new_state = getGotoEntry(parse_stack.top(), t);
					parse_stack.push(new_state);

					terminal_eliminated = true;

					if (this->options.log_step_by_step)
						this->options.logprintln("Parser Trace", "Shift: ", t.stringify(), ", goto ", new_state);

					tree_builder().pushNode(t_);
				} else if (const auto* prod = std::get_if<RuleRef<grammar_type>>(&getActionEntry(parse_stack.top(), t))) { // reduce action
					const typename grammar_type::prod_type& rule = prod->getRule();

					for (int i = 0; i < rule.size(); i++) parse_stack.pop();

					if (!hasGotoEntry(parse_stack.top(), prod->get_name())) {
						this->err_unexpected_token(t_);
						return;
					}

					state_type new_state = getGotoEntry(parse_stack.top(), prod->get_name());
					parse_stack.push(new_state);

					if (this->options.log_step_by_step)
						this->options.logprintln("Parser Trace", "Reduce: ", rule, ", goto ", new_state);

					tree_builder().pushNode(*prod);
				} else if (std::holds_alternative<accept_action>(getActionEntry(parse_stack.top(), t_))) { // accept
					terminal_eliminated = true;

					this->tree_builder().assignToTree(this->out_tree);

				} else { // report error
					this->err_unexpected_token(t_);
				}

			} while (!terminal_eliminated);
		}

		std::set<terminal_type> currently_expected_terminals() const {
			if (parse_stack.empty()) {
				return {{ terminal_type::Type::eof }};
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
				if (const auto* r = std::get_if<RuleRef<grammar_type>>(&getActionEntry(state, std::get<terminal_type>(t)))) {
					const auto& rule = r->getRule();

					if (this->grammar.get_terminal_precs().at(terminal) < rule.prec) {
						return true;
					} else if (this->grammar.get_terminal_precs().at(terminal) == rule.prec) {
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

		BottomUpTreeBuilder<grammar_type> tb;
		TreeBuilder<grammar_type>& tree_builder() { return tb; }

		std::unordered_map<state_type, std::map<symbol_type, state_type>> goto_table;
		std::unordered_map<state_type, std::map<terminal_type, action_type>> action_table;

		state_type initial_state = -1;
		std::stack<state_type> parse_stack;

	};

	template<typename GrammarT = dpl::Grammar<>>
	struct Configuration {
		using grammar_type = GrammarT;

		RuleRef<grammar_type> production;
		int pos = 0;

		Configuration(RuleRef<grammar_type> prod_, int pos_)
			: production(prod_), pos(pos_) { }

		bool operator==(const Configuration<grammar_type>&) const = default;

		auto dot() const {
			return production.getRule().at(pos);
		}

		void next() {
			pos++;
		}

		bool atEnd() const {
			return production.getRule().size() == pos;
		}

		auto getStartConfig() const {
			RuleRef<grammar_type> ruleref(production.get_grammar(), production.get_grammar().get_start_symbol(), 0);
			return Configuration<grammar_type>{ruleref, 0};
		}

		auto toEnd() const {
			auto result = *this;
			result.pos = int(production.getRule().size());
			return result;
		}

		void print_lookahead(std::ostream& os) const {
			// #TASK : fill in
		}

	};

	template<class ConfigT>
	std::vector<ConfigT> computeConfigClosure(typename ConfigT::grammar_type& g, const ConfigT& config) = delete;

	template<typename GrammarT>
	inline std::vector<Configuration<GrammarT>> computeConfigClosure(GrammarT& g, const Configuration<GrammarT>& config) {
		if (config.atEnd()) return {};

		auto symbol = config.dot();
		if (const auto* nonterminal = std::get_if<typename GrammarT::nonterminal_type>(&symbol)) {

			std::vector<Configuration<GrammarT>> result;

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

		using grammar_type = config_type::grammar_type;
		using terminal_type = grammar_type::terminal_type;
		using nonterminal_type = grammar_type::nonterminal_type;

		void computeClosure(grammar_type& g) {
			size_t old_size;
			do {
				old_size = size();

				// iterate through all configurations in state
				for (int i = 0; i < size(); i++) {
					const config_type& config = (*this)[i];
					const typename grammar_type::prod_type& rule = config.production.getRule();

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

		State<ConfigT> successor(std::variant<terminal_type, nonterminal_type> symbol, grammar_type& g) const {
			State<ConfigT> result;

			for (const config_type& config : *this) {
				if (config.atEnd()) continue;

				if (config.dot() == symbol) {
					config_type next_config = config;
					next_config.next();
					result.push_back(next_config);
				}
			}

			result.computeClosure(g);
			return result;
		}

		virtual std::map<terminal_type, typename LR<void>::action_type> getActions(grammar_type& g) const {
			std::map<terminal_type, typename LR<void>::action_type> result;

			for (const config_type& config : *this) {
				if (config == config.getStartConfig().toEnd())
					result[terminal_type::Type::wildcard] = std::monostate{};
				else if (config.atEnd())
					result[terminal_type::Type::wildcard] = config.production;
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


		friend std::ostream& operator<<(std::ostream& os, const State<ConfigT>& state) {
			os << "<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\">\n";

			for (auto iter = state.begin(); iter != state.end(); iter++) {
				const State<ConfigT>::config_type& config = *iter;

				os << "<TR>\n";

				os << "<TD ALIGN=\"left\"";
				if (iter == state.begin()) os << " WIDTH=\"100\"";
				os << ">";

				os << config.production.get_name() << "  &#8594; "; // right arrow

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
		
		using state_type = StateT;
		using grammar_type = state_type::grammar_type;

		using terminal_type = grammar_type::terminal_type;
		using nonterminal_type = grammar_type::nonterminal_type;
		using symbol_type = std::variant<terminal_type, nonterminal_type>;
		using whole_state_type = std::pair<state_type, std::map<symbol_type, int>>;

		std::vector<whole_state_type> states;

		LR0Automaton(grammar_type& g) {
			// augment grammar
			g.augment_start_symbol();
			g.initialize();

			// construct initial state
			state_type start_state;
			start_state.push_back(typename state_type::config_type{ {g, g.get_start_symbol(), 0}, 0});
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
						const typename state_type::config_type& config = states[i].first[j];

						if (config.atEnd()) continue;

						const auto& symbol = config.dot();
						state_type succ = state.successor(symbol, g);

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

		friend std::ostream& operator<<(std::ostream& os, const LR0Automaton<state_type>& automaton) {
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

	template<typename GrammarT = dpl::Grammar<>, typename LexiconT = dpl::Lexicon<>>
	struct LR0 : public LR<LR0Automaton<State<Configuration<GrammarT>>>, GrammarT, LexiconT> {
	private:
		using parent = LR<LR0Automaton<State<Configuration<GrammarT>>>, GrammarT, LexiconT>;
	public:
		using parent::LR;
		constexpr LR0(GrammarT& g, LexiconT& l, Parser<GrammarT, LexiconT>::Options ops = {})
			: parent(g, l, ops) {}
	};
}