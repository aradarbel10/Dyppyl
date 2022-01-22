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


	/*auto [grammar, lexicon] = (
		dpl::discard	|= dpl::Lexeme{ dpl::kleene{dpl::whitespace} },

		"S"nt			|= "OS"nt | "CS"nt,
		"OS"nt			|= ("if"t, "C"nt, "then"t, "SS"nt)
						|  ("if"t, "C"nt, "then"t, "OS"nt)
						|  ("if"t, "C"nt, "then"t, "CS"nt, "else"t, "OS"nt),
		"CS"nt 			|= "SS"nt
						|  ("if"t, "C"nt, "then"t, "CS"nt, "else"t, "CS"nt),
		"SS"nt			|= "OS'"nt,
		"OS'"nt 		|= ";"t,
		"C"nt 			|= "t"t | "f"t
	);*/

	auto [grammar, lexicon] = (
		dpl::discard	|= dpl::Lexeme{ dpl::kleene{dpl::whitespace} },

		"S"nt			|= "OS"nt | "CS"nt,

		"OS"nt			|= ("if"t, "C"nt, "then"t, "S"nt)
						|  ("if"t, "C"nt, "then"t, "CS"nt, "else"t, "OS"nt),

		"CS"nt 			|= "NON_IF"nt
						|  ("if"t, "C"nt, "then"t, "CS"nt, "else"t, "CS"nt),

		"NON_IF"nt 		|= ";"t,

		"C"nt 			|= !"t"t | !"f"t
	);

	dpl::SLR parser{ grammar, lexicon };
	auto [tree, errors] = parser.parse("if t then if t then ; else ;");
	
	std::cout << tree;

	return 0;
}