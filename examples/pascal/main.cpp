#include <dyppyl.h>

#include "treewalk.h"

int main() {
	using namespace dpl::literals;

	auto [grammar, lexicon] = (
		dpl::discard	|= dpl::kleene{dpl::whitespace},

		"name"t			|= dpl::some{dpl::alpha},
		"num"t			|= dpl::some{dpl::digit},
		"add_op"t		|= dpl::any_of{"+-"},
		"mul_op"t		|= dpl::any_of{"*/"},
		"comp"t			|= dpl::alternatives{
											dpl::match{"<>"},
											dpl::match{">="},
											dpl::match{"<="},
											dpl::match{"<"},
											dpl::match{"="},
											dpl::match{">"}
										},


		"program"nt		|= (!"block"nt, ~"."t),
		"block"nt		|= ("decls"nt, "stmt?"nt),

		"decls"nt		|= ("decl"nt, *"decls"nt) | dpl::epsilon,
		"decl"nt		|= !"const"nt | !"var"nt | !"func"nt,

		"const"nt		|= (!"CONST"t, "const_def"nt, *"const_defs"nt, ~";"t),
		"const_defs"nt	|= (~","t, "const_def"nt, *"const_defs"nt) | dpl::epsilon,
		"const_def"nt	|= ("name"t, !"="t, "num"t),

		"var"nt			|= (!"VAR"t, "name"t, *"names"nt, ~";"t),
		"names"nt		|= (~","t, "name"t, *"names"nt) | dpl::epsilon,

		"func"nt		|= (!"FUNCTION"t, "name"t, ~"("t, "names_list"nt, ~")"t, "block"nt, ~";"t),
		"names_list"nt	|= ("name"t, *"names"nt) | dpl::epsilon,

		"expr"nt		|= (!"NOT"t, "expr"nt)
						|  ("expr"nt, !"mul_op"t, "expr"nt) & dpl::Prec{ 5 } & dpl::Assoc::Left
						|  ("expr"nt, !"add_op"t, "expr"nt) & dpl::Prec{ 4 } & dpl::Assoc::Left
						|  ("expr"nt, !"comp"t, "expr"nt) & dpl::Prec{ 3 } & dpl::Assoc::Left
						|  ("expr"nt, !"AND"t, "expr"nt) & dpl::Prec{ 2 } & dpl::Assoc::Left
						|  ("expr"nt, !"OR"t, "expr"nt) & dpl::Prec{ 1 } & dpl::Assoc::Left
						|  (!"ODD"t, "expr"nt)
						|  (!"name"t)
						|  (!"name"t, ~"("t, "exprs_list"nt, ~")"t)
						|   !"num"t
						|  (~"("t, !"expr"nt, ~")"t),
		"exprs_list"nt	|= ("expr"nt, *"exprs"nt) | dpl::epsilon,
		"exprs"nt		|= (~","t, "expr"nt, *"exprs"nt) | dpl::epsilon,

		"stmt?"nt		|= !"stmt"nt | dpl::epsilon,
		"stmt"nt		|= ("name"t, !":="t, "expr"nt)
						|  (!"BEGIN"t, *"stmts_list"nt, ~"END"t)
						|  (!"IF"t, "expr"nt, ~"THEN"t, "stmt"nt)
						|  (~"IF"t, "expr"nt, ~"THEN"t, "stmt"nt, ~"ELSE"t, "stmt"nt) & dpl::Prec{1} // avoid dangling else
						|  (!"WHILE"t, "expr"nt, ~"DO"t, "stmt"nt)
						|  (!"RETURN"t, "expr"nt)
						|  (!"WRITE"t, "expr"nt),
		"stmts_list"nt	|= ("stmt"nt, *"stmts"nt),
		"stmts"nt		|= (~";"t, "stmt"nt, *"stmts"nt) | dpl::epsilon
	);


	dpl::SLR parser{ grammar, lexicon, {
		.log_errors = true,
		.log_tokenizer = true,
	}};

	/*auto [tree, errors] = parser.parse(
R"s(


CONST A = 8, B = 2;
VAR X, Y;

FUNCTION myfunc(A, B)
VAR Asqrd, Bsqrd;
BEGIN
	Asqrd := A * A;
	Bsqrd := B * B;

	IF A >= B THEN
		RETURN (Asqrd + Bsqrd);
	IF A <  B THEN
		RETURN (Asqrd - Bsqrd)
END;

BEGIN
	X := A ;
	Y := 8 ;
	IF X < 42 THEN BEGIN
		Y := myfunc(X - 2 * Y, Y);
		X := A
	END
END.


)s");*/

	auto [tree, errors] = parser.parse(R"s(
		VAR counter, prev, curr, temp;

		BEGIN
			counter := 0;
			prev := 0;
			curr := 1;

			WHILE (counter <= 30) DO BEGIN
				temp := prev + curr;
				prev := curr;
				curr := temp;
				WRITE prev;

				counter := counter + 1
			END
		END.
	)s");

	if (!errors.empty()) return -1;

	std::cout << "\n\n\nAST:" << tree;

	TreeWalker interpreter;
	interpreter.interpret_tree(tree);

	return 0;
}