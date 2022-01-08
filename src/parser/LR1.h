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

#include "../tokenizer/Token.h"
#include "../Grammar.h"
#include "../tokenizer/Tokenizer.h"
#include "LR.h"

namespace dpl {

	template<typename GrammarT = dpl::Grammar<>>
	struct LR1Configuration : public Configuration<GrammarT> {
		using grammar_type = GrammarT;
		using terminal_type = grammar_type::terminal_type;

		terminal_type lookahead;

		LR1Configuration(RuleRef<grammar_type> prod_, int pos_, terminal_type t_)
			: Configuration<grammar_type>(prod_, pos_), lookahead(t_) { }

		friend bool operator==(const LR1Configuration<grammar_type>& lhs, const LR1Configuration<grammar_type>& rhs) {
			return std::tie(lhs.production, lhs.pos, lhs.lookahead) == std::tie(rhs.production, rhs.pos, rhs.lookahead);
		}

		auto toEnd() const {
			auto result = *this;
			result.pos = this->production.getRule().size();
			return result;
		}

		static auto getStartConfig(const grammar_type& g) {
			auto result = LR1Configuration<grammar_type>{
				{g, g.get_start_symbol(), 0},
				0,
				terminal_type{ terminal_type::Type::eof } };
			return result;
		}

		void print_lookahead(std::ostream& os) const {
			os << "<TD>" << lookahead << "</TD>\n";
		}
	};

	template<typename GrammarT>
	inline std::vector<LR1Configuration<GrammarT>> computeConfigClosure(GrammarT& g, const LR1Configuration<GrammarT>& config) {
		if (config.atEnd()) return {};

		auto symbol = config.dot();
		if (const auto* nonterminal = std::get_if<typename GrammarT::nonterminal_type>(&symbol)) {
			std::vector<LR1Configuration<GrammarT>> result;
			const typename GrammarT::prod_type& rule = g[config.production.get_name()][config.production.get_prod()];

			for (int i = 0; i < g.at(*nonterminal).size(); i++) {
				typename GrammarT::prod_type rule_for_first(std::next(rule.begin(), config.pos + 1), rule.end());
				rule_for_first.push_back(config.lookahead);
				auto first_star_set = g.first_star(rule_for_first);

				for (const auto& terminal : first_star_set) {
					if (!std::holds_alternative<std::monostate>(terminal)) {
						const LR1Configuration<GrammarT> new_config = { { g, *nonterminal, i }, 0, std::get<typename GrammarT::terminal_type>(terminal) };
						result.push_back(new_config);
					}
				}
			}

			return result;
		} else return {};
	}

	template<typename GrammarT = dpl::Grammar<>>
	struct LR1State : public State<LR1Configuration<GrammarT>> {

		using grammar_type = GrammarT;

		using state_type = int;
		using accept_action = std::monostate;
		using terminal_type = grammar_type::terminal_type;

		using action_type = std::variant<accept_action, state_type, RuleRef<grammar_type>>;

		LR1State() = default;
		LR1State(const State<LR1Configuration<GrammarT>>& other) : State<LR1Configuration<GrammarT>>(other) { }


		virtual std::map<terminal_type, action_type> getActions(grammar_type& g) const override {
			std::map<terminal_type, action_type> result;

			auto end_config = LR1Configuration<grammar_type>::getStartConfig(g).toEnd();

			for (const LR1Configuration<grammar_type>& config : *this) {
				if (config == end_config)
					result[config.lookahead] = accept_action{};
				else if (config.atEnd())
					result[config.lookahead] = config.production;
			}

			return result;
		}

	};


	template<typename GrammarT = dpl::Grammar<>, typename LexiconT = dpl::Lexicon<>>
	struct LR1 : public LR<LR0Automaton<LR1State<GrammarT>>, GrammarT, LexiconT> {
	private:
		using parent = LR<LR0Automaton<LR1State<GrammarT>>, GrammarT, LexiconT>;
	public:
		using parent::LR;
		constexpr LR1(GrammarT& g, LexiconT& l, Parser<GrammarT, LexiconT>::Options ops = {})
			: parent(g, l, ops) {}
	};
}