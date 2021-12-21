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

	struct LR1Configuration : public Configuration {
		Terminal lookahead;

		LR1Configuration(RuleRef prod_, int pos_, Terminal t_)
			: Configuration(prod_, pos_), lookahead(t_) { }

		friend bool operator==(const LR1Configuration& lhs, const LR1Configuration& rhs) {
			return std::tie(lhs.production, lhs.pos, lhs.lookahead) == std::tie(rhs.production, rhs.pos, rhs.lookahead);
		}

		static LR1Configuration getStartConfig(Grammar& g) {
			return LR1Configuration{ {g, g.start_symbol, 0}, 0, Terminal::Type::EndOfFile };
		}

		void print_lookahead(std::ostream& os) const {
			os << "<TD>" << lookahead << "</TD>\n";
		}
	};

	template<>
	inline std::vector<LR1Configuration> computeConfigClosure(Grammar& g, const LR1Configuration& config) {
		if (config.atEnd(g)) return {};

		auto symbol = config.dot(g);
		if (const auto* nonterminal = std::get_if<std::string_view>(&symbol)) {
			std::vector<LR1Configuration> result;
			const ProductionRule& rule = g[config.production.name][config.production.prod];

			for (int i = 0; i < g[*nonterminal].size(); i++) {
				ProductionRule rule_for_first(std::next(rule.begin(), config.pos + 1), rule.end());
				rule_for_first.push_back(config.lookahead);
				auto first_star_set = g.first_star(rule_for_first);

				for (const auto& terminal : first_star_set) {
					if (!std::holds_alternative<std::monostate>(terminal)) {
						const LR1Configuration new_config = { { g, *nonterminal, i }, 0, std::get<Terminal>(terminal) };
						result.push_back(new_config);
					}
				}
			}

			return result;
		} else return {};
	}



	struct LR1State : public State<LR1Configuration> {

		LR1State() = default;
		LR1State(const State<LR1Configuration>& other) : State<LR1Configuration>(other) { }


		virtual std::map<typename LR<void>::terminal_type, typename LR<void>::action_type> getActions(Grammar& g) const override {
			std::map<typename LR<void>::terminal_type, typename LR<void>::action_type> result;

			for (const LR1Configuration& config : *this) {
				if (config == LR1Configuration::getStartConfig(g).toEnd(g))
					result[config.lookahead] = std::monostate();
				else if (config.atEnd(g))
					result[config.lookahead] = config.production;
			}

			return result;
		}

	};


	typedef LR<LR0Automaton<LR1State>> LR1;
}