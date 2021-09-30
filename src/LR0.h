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

#include "parser/LR.h"

#include "Token.h"
#include "Grammar.h"

namespace dpl {

	class LR0 : public LR {
	public:

		using state_type = int;
		using accept_action = std::monostate;

		using out_type = std::variant<Token, std::pair<std::string_view, int>>;
		using symbol_type = std::variant<std::monostate, Token, std::string_view>;

		using action_type = std::variant<accept_action, state_type, std::pair<std::string_view, int>>;

		LR0(Grammar& g, ParseTree& pt, Tokenizer& inp) : LR(g, pt, inp) {
			//LR0Automaton automaton{ g };

			// generate parse tables
			//for (int i = 0; i < automaton.states.size(); i++) {
			//	auto& [state, transes] = automaton.states[i];

			//	for (const auto& config : state) {
			//		if (config.production.first == g.start_symbol
			//			&& config.production.second == 0
			//			&& config.pos == 1)
			//		{
			//			if (hasActionEntry(i, Token::Type::Unknown)) std::cout << "Error: Duplicate Action Entries -- Non-LR(1) Grammar!\n";
			//			else action_table[i][Token::Type::Unknown] = accept_action();

			//			initial_state = i;
			//		} else if (config.atEnd(g)) {
			//			if (hasActionEntry(i, Token::Type::Unknown)) std::cout << "Error: Duplicate Action Entries -- Non-LR(1) Grammar!\n";
			//			else action_table[i][Token::Type::Unknown] = config.production;
			//		}
			//	}

			//	for (const auto& [symbol, dest] : transes) {
			//		if (hasGotoEntry(i, symbol)) std::cout << "Error: Duplicate Action Entries -- Non-LR(1) Grammar!\n";
			//		else goto_table[i][symbol] = dest;
			//	}
			//}
			//

			//parse_stack.push(0);
		}

		void operator<<(const Token& t) {
			//Token t_ = getTerminalType(t);

			//bool terminal_eliminated = false;
			//do {
			//	
			//	if (!hasActionEntry(parse_stack.top(), t_)) { // not contains means the action is shift
			//		if (!hasGotoEntry(parse_stack.top(), t_)) {
			//			std::cerr << "Syntax error: unexptected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
			//			return;
			//		}

			//		int new_state = goto_table[parse_stack.top()][t_];
			//		parse_stack.push(new_state);

			//		terminal_eliminated = true;

			//		std::cout << "Shift: " << t.stringify() << ", goto " << new_state << '\n';
			//		tree_builder.addSubTree(t);
			//	} else if (const auto* prod = std::get_if<std::pair<std::string_view, int>>(&getActionEntry(parse_stack.top(), t_))) { // reduce action
			//		const ProductionRule& rule = grammar[(*prod).first][(*prod).second];

			//		for (int i = 0; i < rule.size(); i++) parse_stack.pop();

			//		if (!hasGotoEntry(parse_stack.top(), (*prod).first)) {
			//			std::cerr << "Syntax error: unexptected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
			//			return;
			//		}

			//		int new_state = goto_table[parse_stack.top()][(*prod).first];
			//		parse_stack.push(new_state);

			//		std::cout << "Reduce: " << rule << ", goto " << new_state << '\n';
			//		tree_builder.packTree(*prod, rule.size());
			//	} else if (std::holds_alternative<std::monostate>(getActionEntry(parse_stack.top(), t_))) { // accept
			//		terminal_eliminated = true;
			//		tree_builder.assignToTree(out_tree);
			//	} else { // report error
			//		std::cerr << "Syntax error: unexpected token " << t.stringify() << " at position (" << dpl::log::streamer{ t.pos } << ")\n";
			//	}

			//} while (!terminal_eliminated);
		}

	private:

		//bool hasGotoEntry(const int state, const typename symbol_type& t) {
		//	return goto_table.contains(state) && goto_table[state].contains(t);
		//}

		//bool hasActionEntry(const int state, const Token& t) {
		//	return action_table.contains(state);//&& action_table[state].contains(t);
		//}

		//action_type& getActionEntry(const int state, const Token& t) {
		//	return action_table[state][Token::Type::Unknown];
		//}

		//Tokenizer& input;
		//Grammar& grammar;
		//BottomUpTreeBuilder tree_builder;
		//ParseTree& out_tree;

		//std::unordered_map<int, std::map<symbol_type, int>> goto_table;
		////std::unordered_map<int, std::variant<std::monostate, std::pair<std::string_view, int>>> action_table;
		//std::unordered_map<int, std::map<Token, action_type>> action_table;

		//int initial_state = -1;
		//std::stack<int> parse_stack;

	};
}