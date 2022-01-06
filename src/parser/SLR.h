#pragma once

#include "LR.h"

namespace dpl {
	template<typename ConfigT>
	struct SLRState : public State<ConfigT> {
	private:
		using parent = State<ConfigT>;

	public:
		using config_type = ConfigT;
		using grammar_type = config_type::grammar_type;

		using state_type = int;
		using accept_action = std::monostate;
		using terminal_type = grammar_type::terminal_type;

		using action_type = std::variant<accept_action, state_type, RuleRef<grammar_type>>;

		SLRState() = default;
		SLRState(const State<Configuration<grammar_type>>& other) : State<Configuration<grammar_type>>(other) {}

		virtual std::map<terminal_type, action_type> getActions(grammar_type& g) const override {
			std::map<terminal_type, action_type> result;

			auto end_config = Configuration<grammar_type>{ RuleRef<grammar_type>{g, g.get_start_symbol(), 0}, 0 };
			end_config = end_config.toEnd();

			for (const config_type& config : *this) {
				if (config == end_config) {
					result[terminal_type::Type::eof] = accept_action{};
				} else if (config.atEnd()) {
					const auto& follow = g.get_follows().at(config.production.get_name());
					for (const auto& terminal : follow) {
						result[terminal] = config.production;
					}
				}
			}

			return result;
		}
	};

	template<typename GrammarT = dpl::Grammar<>, typename LexiconT = dpl::Lexicon<>>
	struct SLR : public LR<LR0Automaton<SLRState<Configuration<GrammarT>>>, GrammarT, LexiconT> {
	private:
		using parent = LR<LR0Automaton<SLRState<Configuration<GrammarT>>>, GrammarT, LexiconT>;
	public:
		using parent::LR;
		constexpr SLR(GrammarT& g, LexiconT& l, Parser<GrammarT, LexiconT>::Options ops = {})
			: parent(g, l, ops) {}
	};
}