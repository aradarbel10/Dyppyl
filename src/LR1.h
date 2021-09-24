#pragma once

#include <string_view>
#include <vector>

#include "Token.h"
#include "Grammar.h"

namespace dpl {

	template<typename KwdT, typename SymT>
	class LR1Automata {
	public:

		using Token = Token<KwdT, SymT>;
		using Transition = std::pair<Token, size_t>;
		using Grammar = Grammar<KwdT, SymT>;

	private:

		struct State {
			std::pair<std::string_view, int> production;
			int pos;
			Token terminal;
		};

	public:

		std::vector<std::pair<State, std::list<Transition>>> states;

		LR1Automata(Grammar& g) {
			// augment grammar
			// construct initial state
			// add all states and transitions based on alg.
		}

	};


	template<typename KwdT, typename SymT>
	class LR1 {
	public:

		using Token = Token<KwdT, SymT>;
		using ProductionRule = ProductionRule<KwdT, SymT>;
		using Nonterminal = Nonterminal<KwdT, SymT>;
		using Grammar = Grammar<KwdT, SymT>;
		using ParseTree = ParseTree<KwdT, SymT>;
		using out_type = std::variant<Token, std::pair<std::string_view, int>>;

		LR1(Grammar& g, std::string_view s, ParseTree& pt) : grammar(g), start_symbol(s), out_tree(pt) {
			// generate parse tables
			//	- action table
			//	- transition table
		}

		void fetchNext() {

		}

		void generateParseTables() {
			
		}

	private:

		std::string_view start_symbol;
		Grammar& grammar;
		ParseTree& out_tree;

	};
}