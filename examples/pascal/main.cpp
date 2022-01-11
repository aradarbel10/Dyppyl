#include <dyppyl.h>

#include "treewalk.h"

// soem helper functions to use when transforming the parse tree
bool is_punctuation(const dpl::Token<>& tkn, const dpl::Lexicon<>& lexicon) {
	// a token is considered punctuation if it doesn't hold any semantic information (it's value is just it's type without any other parameters)
	return std::holds_alternative<dpl::match<>>(lexicon.at(tkn.name).regex.get_variant());
}

std::optional<dpl::Terminal<>> get_first_terminal(const dpl::RuleRef<>& rule, const dpl::Lexicon<>& lexicon) {
	for (const auto& symbol : rule.getRule()) {
		auto* terminal = std::get_if<dpl::Terminal<>>(&symbol);

		// if token represents punctuation
		if (terminal && std::holds_alternative<dpl::match<>>(lexicon.at(terminal->name).regex.get_variant()))
			return *terminal;
	}
	return std::nullopt;
}

int count_nodes(const dpl::ParseTree<>& tree) {
	int count = 1;
	for (const auto& child : tree.children) {
		count += count_nodes(child);
	}
	return count;
}

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

	auto [tree, errors] = parser.parse(
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


)s");

	if (!errors.empty()) return -1;


	//dpl::tree_visit(tree, [&](dpl::ParseTree<>& subtree) {
	//	// ignore empty
	//	if (subtree.children.empty()) return;

	//	// traversing bottom-up so all inner nodes should be rule refs
	//	const auto rule = std::get<dpl::RuleRef<>>(subtree.value);

	//	// rename tree node (!)
	//	auto first_terminal = get_first_terminal(rule, lexicon);
	//	if (first_terminal) {
	//		dpl::Token<> token{ *first_terminal };
	//		token.value = token.name;
	//		subtree.value = token;
	//	} else subtree.value = rule.get_name();

	//	int index_in_prod = -1;
	//	for (int i = 0; i < subtree.children.size(); i++) {
	//		// keep track of the actual index in the production
	//		index_in_prod++;

	//		auto* token = std::get_if<dpl::Token<>>(&subtree[i].value);
	//		auto* childrule = std::get_if<dpl::RuleRef<>>(&subtree[i].value);

	//		// remove redundant punctuations and epsilon productions
	//		if (token && is_punctuation(*token, lexicon) && subtree[i].children.empty() || childrule && childrule->getRule().empty()) {
	//			subtree.children.erase(subtree.children.begin() + i);
	//			i--;
	//			continue;
	//		}
	//	}

	//	// lift only-child
	//	if (subtree.children.size() == 1) {
	//		auto child = subtree[0];
	//		subtree = child;
	//	}
	//});


	std::cout << "\n\n\nAST (" << count_nodes(tree) << ")\n" << tree;

	TreeWalker interpreter;
	interpreter.interpret_tree(tree);

	return 0;
}