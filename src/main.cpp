#include <iostream>
#include <algorithm>
#include <memory>
#include <string_view>
#include <variant>
#include <unordered_map>
#include <charconv>


//#include "dyppyl.h"

#include "Regex.h"
#include "tokenizer/Token.h"
#include "Lexical.h"
#include "tokenizer/Tokenizer.h"

#include "Grammar.h"


#include "parser/LL1.h"
#include "parser/LR.h"
#include "parser/LR1.h"
#include "parser/SLR.h"
#include "parser/LALR.h"



int main() {

	using namespace dpl::literals;
	using namespace std::string_view_literals;


	auto [grammar, lexicon] = (
		dpl::discard	|= dpl::Lexeme{ dpl::kleene{dpl::whitespace} },
		"E"nt			|= (!"int"t)
						|  (!"func"t, ~"("t, *"Es"nt, ~")"t)
						|  (~"("t, "E"nt, !"Op"nt, "E"nt, ~")"t),
		"Es"nt			|= ("E"nt, *"Es"nt) | dpl::epsilon,
		"Op"nt			|= !"+"t | !"*"t
	);

	dpl::LL1 parser{ grammar, lexicon };
	auto [tree, errors] = parser.parse("(func((int+(int*int)) int func((int * int))) + int)");
	
	std::cout << tree;

	return 0;
}