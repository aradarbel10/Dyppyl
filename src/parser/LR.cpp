#include "LR.h"

//#include "../Token.h"
//#include "../Logger.h"

namespace dpl {

	LR::LR(Grammar& g, ParseTree& pt, Tokenizer& inp) : input(inp), grammar(g), out_tree(pt), tree_builder(grammar) {
		LR0Automaton automaton{ g };

		// generate parse tables
		for (int i = 0; i < automaton.states.size(); i++) {
			auto& [state, transes] = automaton.states[i];

			for (const auto& config : state) {
				if (config.production.first == g.start_symbol
					&& config.production.second == 0
					&& config.pos == 1)
				{
					if (hasActionEntry(i, Token::Type::Unknown)) std::cout << "Error: Duplicate Action Entries -- Non-LR(1) Grammar!\n";
					else action_table[i][Token::Type::Unknown] = accept_action();

					initial_state = i;
				} else if (config.atEnd(g)) {
					if (hasActionEntry(i, Token::Type::Unknown)) std::cout << "Error: Duplicate Action Entries -- Non-LR(1) Grammar!\n";
					else action_table[i][Token::Type::Unknown] = config.production;
				}
			}

			for (const auto& [symbol, dest] : transes) {
				if (hasGotoEntry(i, symbol)) std::cout << "Error: Duplicate Action Entries -- Non-LR(1) Grammar!\n";
				else goto_table[i][symbol] = dest;
			}
		}


		parse_stack.push(0);
	}

	void LR::operator<<(const Token& t) {
		Token t_ = getTerminalType(t);

		bool terminal_eliminated = false;
		do {

			if (!hasActionEntry(parse_stack.top(), t_)) { // not contains means the action is shift
				if (!hasGotoEntry(parse_stack.top(), t_)) {
					std::cerr << "Syntax error: unexptected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
					return;
				}

				int new_state = goto_table[parse_stack.top()][t_];
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

				int new_state = goto_table[parse_stack.top()][(*prod).first];
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
}