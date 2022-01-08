#pragma once

#include "LR1.h"

namespace dpl {
	template<typename GrammarT = dpl::Grammar<>>
	class LALRAutomaton {
	public:
		using grammar_type = GrammarT;

		using terminal_type = grammar_type::terminal_type;
		using nonterminal_type = grammar_type::nonterminal_type;
		using symbol_type = std::variant<terminal_type, nonterminal_type>;

		using lr0_automaton_type = dpl::LR0Automaton<dpl::State<dpl::Configuration<grammar_type>>>;
		using nt_transition_type = std::pair<int, nonterminal_type>;

		using augmented_grammar_type = dpl::Grammar<typename grammar_type::token_type, nt_transition_type, std::string_view>;

	private:
		lr0_automaton_type lr0_automaton;
		std::map<std::pair<int, RuleRef<grammar_type>>, typename grammar_type::follows_type::mapped_type> lookaheads;

		augmented_grammar_type::prod_type spell_production(const lr0_automaton_type& lr0_automaton, int spell_state, const typename grammar_type::prod_type& prod) {
			typename augmented_grammar_type::prod_type result{};
			
			for (const auto& orig_sym : prod) {
				if (auto* terminal = std::get_if<terminal_type>(&orig_sym)) {
					result.push_back(*terminal);
				} else if (auto* nonterminal = std::get_if<nonterminal_type>(&orig_sym)) {
					result.push_back(nt_transition_type{ spell_state, *nonterminal });
				}

				spell_state = lr0_automaton.states.at(spell_state).second.at(orig_sym);
			}

			return result;
		}

		auto end_of_spelling(const lr0_automaton_type& lr0_automaton, int spell_state, const typename grammar_type::prod_type& prod) {
			for (const auto& orig_sym : prod) {
				spell_state = lr0_automaton.states.at(spell_state).second.at(orig_sym);
			}

			return spell_state;
		}

		template<typename GrammarT = dpl::Grammar<>>
		struct VirtualState {
		private:
			using grammar_type = GrammarT;
			using accept_action = std::monostate;
			using action_type = std::variant<accept_action, int, RuleRef<grammar_type>>;

			const LALRAutomaton<grammar_type>* automaton = nullptr;
			size_t index;
		public:
			VirtualState(const LALRAutomaton<grammar_type>* a, size_t i) : automaton(a), index(i) {}

			virtual std::map<terminal_type, action_type> getActions(grammar_type& g) const {
				std::map<terminal_type, action_type> result;

				auto end_config = Configuration<grammar_type>::getStartConfig(g).toEnd();

				for (const auto& config : automaton->lr0_automaton.states[index].first) {
					if (config == end_config) {
						result[terminal_type::Type::eof] = accept_action{};
					} else if (config.atEnd()) {
						const auto& lookahead = automaton->lookaheads.at({ index, config.production });
						for (const auto& terminal : lookahead) {
							result[terminal] = config.production;
						}
					}
				}

				return result;
			}
		};

		template<typename GrammarT = dpl::Grammar<>>
		struct VirtualStates {
		private:
			using grammar_type = GrammarT;

			const LALRAutomaton<grammar_type>* automaton = nullptr;
		public:
			VirtualStates(const LALRAutomaton<grammar_type>* a) : automaton(a) {}
			size_t size() const {
				return automaton->lr0_automaton.states.size();
			}
			auto operator[](size_t index) const {
				const auto& transes = automaton->lr0_automaton.states[index].second;
				return std::pair{ VirtualState<grammar_type>{ automaton, index }, transes };
			}
		};
	public:
		VirtualStates<grammar_type> states{ this };

		LALRAutomaton(grammar_type& grammar) : lr0_automaton(grammar) {
			// construct augmented grammar
			augmented_grammar_type augmented_grammar;

			augmented_grammar.start_symbol = nt_transition_type{0, std::get<nonterminal_type>(grammar.ntrules.at(grammar.get_start_symbol()).front()[0])};

			for (int i = 0; i < lr0_automaton.states.size(); i++) {
				auto& [state, transes] = lr0_automaton.states[i];

				for (const auto& [symbol, dest] : transes) {
					auto* nonterminal = std::get_if<nonterminal_type>(&symbol);
					if (!nonterminal) continue;

					augmented_grammar.ntrules.insert({ nt_transition_type{i, *nonterminal}, {nt_transition_type{i, *nonterminal}, {}} });

					for (const auto& orig_prod : grammar.ntrules.at(*nonterminal)) {
						auto spelled = spell_production(lr0_automaton, i, orig_prod);
						augmented_grammar.ntrules[nt_transition_type{ i, *nonterminal }].push_back(spelled);
					}
				}
			}

			// calculate follow sets of augmented grammar
			augmented_grammar.initialize();

			// compute lookahead sets for each state
			for (int i = 0; i < lr0_automaton.states.size(); i++) {
				auto& [state, transes] = lr0_automaton.states[i];

				for (const auto& config : state) {
					if (config.pos == 0) {
						nt_transition_type Aqr{ i, config.production.get_name() };
						if (Aqr.second == grammar.start_symbol) continue;

						auto dest_state = end_of_spelling(lr0_automaton, i, config.production.getRule());

						for (const auto& terminal : augmented_grammar.follows.at(Aqr)) {
							lookaheads[{dest_state, config.production}].insert(terminal);
						}
					}
				}
			}
		}

		friend std::ostream& operator<<(std::ostream& os, const LALRAutomaton<grammar_type>& automaton) {
			os << automaton.lr0_automaton;
			return os;
		}
	};

	
	template<typename GrammarT = dpl::Grammar<>, typename LexiconT = dpl::Lexicon<>>
	struct LALR : public LR<LALRAutomaton<GrammarT>, GrammarT, LexiconT> {
	private:
		using parent = LR<LALRAutomaton<GrammarT>, GrammarT, LexiconT>;
	public:
		using parent::LR;
		constexpr LALR(GrammarT& g, LexiconT& l, Parser<GrammarT, LexiconT>::Options ops = {})
			: parent(g, l, ops) {}
	};
}