#include "Regex.h"
#include "tokenizer/Token.h"
#include "Lexical.h"
#include "tokenizer/Tokenizer.h"

#include "Grammar.h"
#include "TextStream.h"

#include "parser/LL1.h"

int main() {
	using namespace dpl::literals;

	auto [grammar, lexicon] = (
		dpl::discard	|= dpl::Lexeme{dpl::kleene{dpl::whitespace}},

		"name"t			|= dpl::Lexeme{dpl::some{dpl::alpha}},
		"num"t			|= dpl::Lexeme{dpl::some{dpl::digit}},
		"add_op"t		|= dpl::Lexeme{dpl::any_of{"+-"}},
		"mul_op"t		|= dpl::Lexeme{dpl::any_of{"*/"}},


		"program"nt		|= ("block"nt, "."t),
		"block"nt		|= ("decls"nt, "stmt?"nt),

		"decls"nt		|= ("decl"nt, "decls"nt) | dpl::epsilon,
		"decl"nt		|= "const"nt | "var"nt | "func"nt,

		"const"nt		|= ("CONST"t, "const_def"nt, "const_defs"nt, ";"t),

		"const_defs"nt	|= (","t, "const_def"nt, "const_defs"nt) | dpl::epsilon,
		"const_def"nt	|= ("name"t, "="t, "num"t),

		"var"nt			|= ("VAR"t, "name"t, "names"nt, ";"t),
		"names"nt		|= (","t, "name"t, "names"nt) | dpl::epsilon,

		"func"nt		|= ("FUNCTION"t, "name"t, "("t, "names_list"nt, ")"t, "block"nt, ";"t),
		"names_list"nt	|= ("name"t, "names"nt) | dpl::epsilon,

		"expr"nt		|= ("add_op?"nt, "term"nt, "terms"nt),
		"add_op?"nt		|=  "add_op"t | dpl::epsilon,
		"terms"nt		|= ("add_op"t, "term"nt, "terms"nt) | dpl::epsilon,

		"term"nt		|= ("factor"nt, "factors"nt),
		"factors"nt		|= ("mul_op"t, "factor"nt, "factors"nt) | dpl::epsilon,

		"factor"nt		|= ("name"t, "params"nt)
						|  "num"t
						|  ("("t, "expr"nt, ")"t),
		"params"nt		|= ("("t, "exprs_list"nt, ")"t) | dpl::epsilon,
		"exprs_list"nt	|= ("expr"nt, "exprs"nt) | dpl::epsilon,
		"exprs"nt		|= (","t, "expr"nt, "exprs"nt),

		"stmt?"nt		|= "stmt"nt | dpl::epsilon,
		"stmt"nt		|= ("name"t, ":="t, "expr"nt)
						|  ("BEGIN"t, "stmts_list"nt, "END"t)
						|  ("IF"t, "condition"nt, "THEN"t, "stmt"nt)
						|  ("WHILE"t, "condition"nt, "DO"t, "stmt"nt)
						|  ("RETURN"t, "expr"nt)
						|  ("WRITE"t, "expr"nt),
		"stmts_list"nt	|= ("stmt"nt, "stmts"nt),
		"stmts"nt		|= (";"t, "stmt"nt, "stmts"nt) | dpl::epsilon,

		"condition"nt	|= ("ODD"t, "expr"nt)
						|  ("expr"nt, "comp"nt, "expr"nt),
		"comp"nt		|= "="t | "<>"t | "<"t | "<="t | ">"t | ">="t
	);


	dpl::LL1 parser{ grammar, lexicon, {
		.log_step_by_step = true,
		.log_parse_tree = true,
		.log_errors = true,
		.log_tokenizer = true,

		.log_grammar_info = true,
	}};

	auto [tree, errors] = parser.parse(
R"s(
CONST A = 8, B = 2 ;
VAR X, Y ;
BEGIN
	X := A ;
	Y := 8 ;
	IF X < 42 THEN BEGIN
		Y := X + B ;
		X := A
	END
END.
)s");

	return 0;
}