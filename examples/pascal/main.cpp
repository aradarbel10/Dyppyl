#include "Regex.h"
#include "tokenizer/Token.h"
#include "Lexical.h"
#include "tokenizer/Tokenizer.h"

#include "Grammar.h"
#include "TextStream.h"

#include "parser/LL1.h"
#include "parser/LALR.h"


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

	if (!errors.empty()) return -1;

/*
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
*/

	std::cout << "\n\n\nparse tree (" << count_nodes(tree) << ")\n" << tree;


	dpl::tree_visit(tree, [&](dpl::ParseTree<>& subtree) {
		// ignore empty
		if (subtree.children.empty()) return;

		// lift only-child
		if (subtree.children.size() == 1) {
			auto child = subtree[0];
			subtree = child;

			return;
		}

		// traversing bottom-up so all inner nodes should be rule refs
		const auto rule = std::get<dpl::RuleRef<>>(subtree.value);

		// rename tree node (!)
		auto first_terminal = get_first_terminal(rule, lexicon);
		if (first_terminal) {
			dpl::Token<> token{ *first_terminal };
			token.value = token.name;
			subtree.value = token;
		}
		else subtree.value = rule.get_name();

		int index_in_prod = -1;
		for (int i = 0; i < subtree.children.size(); i++) {
			// keep track of the actual index in the production
			index_in_prod++;

			auto* token = std::get_if<dpl::Token<>>(&subtree[i].value);
			auto* childrule = std::get_if<dpl::RuleRef<>>(&subtree[i].value);

			// remove redundant punctuations and epsilon productions
			if (token && is_punctuation(*token, lexicon) && subtree[i].children.empty() || childrule && childrule->getRule().empty()) {
				subtree.children.erase(subtree.children.begin() + i);
				i--;
				continue;
			}
		}
	});



	std::cout << "\n\n\nAST (" << count_nodes(tree) << ")\n" << tree;







	std::ofstream fileout;

	fileout.open("LR1_table.html");
	dpl::LR1 lr1_parser{ grammar, lexicon };
	lr1_parser.print_parse_table(fileout);
	fileout.close();

	fileout.open("LALR_table.html");
	dpl::LALR lalr_parser{ grammar, lexicon };
	lalr_parser.print_parse_table(fileout);
	fileout.close();

	return 0;
}