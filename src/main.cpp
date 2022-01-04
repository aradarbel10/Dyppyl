#include <iostream>
#include <algorithm>
#include <memory>
#include <string_view>
#include <variant>
#include <unordered_map>
#include <charconv>

#define DPL_LOG

//#include "dyppyl.h"

#include "Regex.h"
#include "tokenizer/Token.h"
#include "Lexical.h"
#include "tokenizer/Tokenizer.h"

#include "Grammar.h"
#include "TextStream.h"

#include "parser/LL1.h"

using namespace dpl::literals;

#define SYMBOLS_MACRO \
	X(LeftParen, "(") \
	X(RightParen, ")") \
	X(LeftCurly, "{") \
	X(RightCurly, "}") \
	X(LeftSquare, "[") \
	X(RightSquare, "]") \
	X(LeftAngle, "<") \
	X(RightAngle, ">") \
	X(LeftShift, "<<") \
	X(RightShift, ">>") \
	X(LeftRotate, "-<<") \
	X(RightRotate, "->>") \
	X(Tick, "`") \
	X(Tilda, "~") \
	X(Exclamation, "!") \
	X(At, "@") \
	X(Hash, "#") \
	X(Dollar, "$") \
	X(Percentage, "%") \
	X(Caret, "^") \
	X(DoubleCaret, "^^") \
	X(Ampersand, "&") \
	X(DoubleAmpersand, "&&") \
	X(Asterisk, "*") \
	X(DoubleAsterisk, "**") \
	X(Minus, "-") \
	X(MinusMinus, "--") \
	X(Plus, "+") \
	X(PlusPlus, "++") \
	X(UprightSlash, "|") \
	X(DoubleUprightSlash, "||") \
	X(Equal, "=") \
	X(DoubleEqual, "==") \
	X(TripleEqual, "===") \
	X(TildaEqual, "~=") \
	X(ExclamationEqual, "!=") \
	X(PercentageEqual, "%=") \
	X(CaretEqual, "^=") \
	X(AmpersandEqual, "&=") \
	X(AsteriskEqual, "*=") \
	X(MinusEqual, "-=") \
	X(PlusEqual, "+=") \
	X(UprightSlashEqual, "|=") \
	X(SlashEqual, "/=") \
	X(LessEqual, "<=") \
	X(GreaterEqual, ">=") \
	X(Spaceship, "<=>") \
	X(LeftShiftEqual, "<<=") \
	X(RightShiftEqual, ">>=") \
	X(LeftRotateEqual, "-<<=") \
	X(RightRotateEqual, "->>=") \
	X(ColonEqual, ":=") \
	X(Colon, ":") \
	X(DoubleColon, "::") \
	X(Semicolon, ";") \
	X(Comma, ",") \
	X(Period, ".") \
	X(Ellipsis, "...") \
	X(Question, "?") \
	X(Slash, "/") \
	X(Arrow, "->") \
	X(LongArrow, "-->") \
	X(SquigglyArrow, "~>") \
	X(WideArrow, "=>") \
	X(LeftArry, "<-")

#define KEYWORDS_MACRO \
	Y(Int) \
	X(Return, "return") \
	Y(Bitfield) \
	Y(True) \
	Y(False) \
	X(If, "if") \
	X(Then, "then") \
	X(Else, "else") \
	X(While, "while") \
	X(Zero, "zero") \
	X(Not, "not") \
	X(Do, "do") \
	Y(Hello) \
	Y(Heya) \
	Y(Yo) \
	Y(World) \
	Y(PROGRAM) \
	Y(BEGIN) \
	Y(END)





int main() {
	//#define X(name, symbol) name,
	//#define Y(name) name,
	//enum Symbols { SYMBOLS_MACRO };
	//enum Keywords { KEYWORDS_MACRO };
	//#undef X
	//#undef Y

	//#define X(name, symbol) {symbol, Symbols:: name},
	//const std::map<std::string_view, size_t> symbols{ SYMBOLS_MACRO };
	//#undef X
	//
	//#define X(name, str) {str, Keywords:: name},
	//#define Y(name) {#name, Keywords:: name},
	//const std::map<std::string_view, size_t> keywords{ KEYWORDS_MACRO };
	//#undef X
	//#undef Y


	auto [grammar, lexicon] = (
		dpl::discard |= dpl::Lexeme{ dpl::kleene{dpl::whitespace} },
		"Es"nt	|= ("e"t, "Es"nt) | dpl::epsilon
	);

	dpl::Tokenizer tokenizer{ lexicon };

	constexpr std::string_view text = "e e e e";
	auto output = tokenizer.tokenize(text.begin(), text.end());

	std::cout << "tokenizer output:\n";
	for (auto tkn : output) {
		std::cout << tkn << '\n';
	}

	dpl::LL1 parser{ grammar, lexicon, {
		.log_step_by_step		= true,
		.log_parse_tree			= true,
		.log_errors				= true,
		.log_tokenizer			= true,

		.log_grammar			= true,
		.log_grammar_info		= true,

		.log_to_file			= false,
	}};
	auto [tree, errors] = parser.parse(text);

	//dpl::Grammar grammar {
	//	"Stmts"nt	|= ("Stmt"nt, "Stmts"nt)
	//				| dpl::ProductionRule{},

	//	"Stmt"nt	|= ("{"t, "Stmts"nt, "}"t)
	//				| ("if"t, "Expr"nt, "then"t, "Stmt"nt)
	//				| ("while"t, "Expr"nt, "do"t, "Stmt"nt)
	//				| ("assign"t, dpl::Token::Type::Identifier, "="t, "Expr"nt),

	//	"Expr"nt	|= dpl::ProductionRule{ "Term"nt }
	//				| ("zero?"t, "Expr"nt)
	//				| ("not"t, "Expr"nt)
	//				| ("++"t, dpl::Token::Type::Identifier)
	//				| ("--"t, dpl::Token::Type::Identifier)
	//				| ("Expr"nt, "BinOp"nt, "Expr"nt) & dpl::Assoc::Left
	//				| ("("t, "Expr"nt, ")"t),

	//	"Term"nt	|= (dpl::Token::Type::Identifier)
	//				| (dpl::Token::Type::Number),

	//	"BinOp"nt	|= "+"t | "-"t | "*"t | "/"t | "%"t
	//};

	//dpl::LR1 parser{ grammar, {
	//	.log_step_by_step		= true,
	//	.log_parse_tree			= true,
	//	.log_tokenizer			= true,

	//	.log_grammar			= true,
	//	.log_grammar_info		= true,

	//	.log_to_file			= false,
	//}};

	//dpl::StringStream src{
	//	"if not zero? (id1 % id2) then"
	//	"    assign id = constant"
	//	//"while --something do {"
	//	//"    if id then one = two"
	//	//"    if zero? id then tree = four"
	//	//"}"
	//};
	//auto [tree, errors] = parser.parse(src);

	//if (!errors.empty()) std::cout << tree;

	//tree.replace_with(
	//	dpl::ParseTree{ dpl::RuleRef{"E", 0} },
	//	[](const dpl::ParseTree& cs) {
	//		return dpl::ParseTree{ cs[2][0].value, {
	//			(cs[1].match({"E"}) ? cs[1][0].value : cs[1]),
	//			(cs[3].match({"E"}) ? cs[3][0].value : cs[3])
	//		}};
	//	},
	//	dpl::TraverseOrder::BottomUp
	//);

	//std::cout << "\n\nAST:\n" << tree << "\n\n";

	//dpl::tree_visit(tree, [](dpl::ParseTree& tree) {
	//	if (tree.children.size() == 2) {
	//		long double lhs = std::get<long double>(tree[0].get<dpl::Token>().value);
	//		long double rhs = std::get<long double>(tree[1].get<dpl::Token>().value);

	//		if (tree.match({"+"_sym}))
	//			tree = dpl::ParseTree{{ dpl::Token{dpl::Token::Type::Number, lhs + rhs }}};
	//		else if (tree.match({"*"_sym }))
	//			tree = dpl::ParseTree{{ dpl::Token{dpl::Token::Type::Number, lhs * rhs }}};
	//	}
	//});
	//
	//std::cout << "Input String: " << src.getString() << "\n";
	//std::cout << "Result: " << std::get<long double>(tree.get<dpl::Token>().value);
	//std::cout << "\n\n\n";
	//






	//

	//dpl::Grammar grammar2{
	//	"E"nt	|= ("E"nt, "+"t, "E"nt) & dpl::Assoc::Left & dpl::Prec{5}
	//			|  ("E"nt, "*"t, "E"nt) & dpl::Assoc::Left & dpl::Prec{10}
	//			|  ("("t ,"E"nt, ")"t )
	//			|  (dpl::Terminal::Type::Number)
	//};

	//dpl::LR1 parser2{ grammar2 };
	//dpl::StringStream src2{ "1 + 2 * (3 + 4) * 5" };
	//auto [tree2, errors2] = parser2.parse(src2);

	//tree2.replace_with(
	//	dpl::ParseTree{ "E", {{"("_sym}, {}, {")"_sym}}},
	//	[](const dpl::ParseTree& cs) { return cs[1]; }
	//);

	//tree2.replace_with(
	//	dpl::ParseTree{ "E", { {}, {}, {} } },
	//	[](const dpl::ParseTree& cs) {
	//		return dpl::ParseTree{ cs[1].value, {
	//			(cs[0].match({"E"}) ? cs[0][0].value : cs[0]),
	//			(cs[2].match({"E"}) ? cs[2][0].value : cs[2])
	//		}};
	//	}
	//);

	//std::cout << "Input String: " << src2.getString() << "\n";
	//std::cout << "\n\nAST:\n" << tree2;
	

	return 0;
}

#undef SYMBOLS_MACRO
#undef KEYWORDS_MACRO