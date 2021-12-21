#pragma once

#include "LR.h"

namespace dpl {
	struct SLRState : public State<Configuration> {
		SLRState() = default;
		SLRState(const State<Configuration>& other) : State<Configuration>(other) { }

		virtual std::map<typename LR<void>::terminal_type, typename LR<void>::action_type> getActions(Grammar& g) const override {
			std::map<typename LR<void>::terminal_type, typename LR<void>::action_type> result;

			for (const config_type& config : *this) {
				if (config == config_type::getStartConfig(g).toEnd(g)) {
					result[Terminal::Type::EndOfFile] = std::monostate{};
				} else if (config.atEnd(g)) {
					const auto& follow = g.follows[config.production.name];
					for (const auto& terminal : follow) {
						result[terminal] = config.production;
					}
				}
			}

			return result;
		}
	};

	typedef LR<LR0Automaton<SLRState>> SLR;
}