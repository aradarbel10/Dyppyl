#include <iostream>
#include <algorithm>
#include <memory>
#include <string_view>
#include <variant>
#include <unordered_map>

#define DPL_LOG

#include "Logger.h"

#include "tokenizer/Token.h"
#include "tokenizer/Tokenizer.h"
#include "Grammar.h"
#include "parser/LL1.h"
#include "ParseTree.h"
#include "TextStream.h"
#include "parser/LR.h"
#include "parser/LR1.h"

using namespace dpl::literals;

// search this solution for "#TASK" to find places where optimizations/refactoring/improvements may be worth implementing

// #TASK : wrap up tokenizer, parser, tree-gen in a separate "front end" object
// #TASK : find better way to handle exceptions
// #TASK : ensure constexpr-ness of whatever's possible
// #TASK : write tests

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
	#define X(name, symbol) name,
	#define Y(name) name,
	enum Symbols { SYMBOLS_MACRO };
	enum Keywords { KEYWORDS_MACRO };
	#undef X
	#undef Y

	#define X(name, symbol) {symbol, Symbols:: name},
	const std::map<std::string_view, size_t> symbols{ SYMBOLS_MACRO };
	#undef X
	
	#define X(name, str) {str, Keywords:: name},
	#define Y(name) {#name, Keywords:: name},
	const std::map<std::string_view, size_t> keywords{ KEYWORDS_MACRO };
	#undef X
	#undef Y

	dpl::Token::keywords = keywords;
	dpl::Token::symbols = symbols;

	using Token = dpl::Token;

	dpl::Grammar example_grammar{
		{ "Stmts", {
			{ "Stmt", "Stmts" },
			{ }
		}},
		{ "Term", {
			{ dpl::Token::Type::Identifier },
			{ dpl::Token::Type::Number }
		}},
		{ "Expr", {
			{ "Term", "->"_sym, dpl::Token::Type::Identifier },
			{ "zero"_kwd, "Term" },
			{ "not"_kwd, "Expr"},
			{ "++"_sym, dpl::Token::Type::Identifier },
			{ "--"_sym, dpl::Token::Type::Identifier }
		}},
		{ "Stmt", {
			{ "if"_kwd, "Expr" , "then"_kwd, "Block" },
			{ "while"_kwd, "Expr", "do"_kwd, "Block"},
			{ "Expr", ";"_sym}
		}},
		{ "Block", {
			{ "Stmt" },
			{ "{"_sym, "Stmts", "}"_sym }
		}}
	};

	dpl::Grammar precedence_grammar{
		{ "E", {
			{ "T", "E'" }
		}},
		{ "E'", {
			{ "+"_sym, "T", "E'" },
			{ }
		}},
		{ "T", {
			{ "F", "T'" }
		}},
		{ "T'", {
			{ "*"_sym, "F", "T'" },
			{ }
		}},
		{ "F", {
			{ dpl::Token::Type::Identifier },
			{ "("_sym, "E", ")"_sym }
		}}
	};

	dpl::Grammar non_ll1{
		{ "E" , {
			{ "T", ";"_sym },
			{ "T", "+"_sym, "E"}
		}},
		{ "T", {
			{ "Int"_kwd },
			{ "("_sym, "E", ")"_sym }
		}}
	};


	dpl::Grammar non_lr0{
		{ "E" , {
			{ "T" },
			{ "E", "+"_sym, "T"}
		}},
		{ "T", {
			{ "Int"_kwd },
			{ "("_sym, "E", ")"_sym }
		}},
	};

	dpl::Grammar lr1_test{
		{ "S", {
			{ "C", "C" }
		}},
		{ "C", {
			{ "+"_sym, "C" },
			{ "-"_sym }
		}}
	};



	dpl::Grammar lr1_epsilon{
		{ "Program", {
			{ "PROGRAM"_kwd, Token::Type::Identifier, "BEGIN"_kwd, "Stmts", "END"_kwd }
		}},
		{ "Stmts", {
			{ "Stmts", "+"_sym },
			{ }
		}}
	};


	dpl::Grammar PascalLikeGrammar{
		{ "Program", {
			{ "PROGRAM"_kwd, Token::Type::Identifier, "BEGIN"_kwd, "Stmts", "END"_kwd }
		}},
		{ "Stmts", {
			{ "Stmts", "Stmt" },
			{ }
		}},
		{ "Stmt", {
			{ Token::Type::Identifier, ":="_sym, "Rvalue", ";"_sym }
		}},
		{ "Rvalue", {
			{ Token::Type::Number },
			{ Token::Type::String },
			{ Token::Type::Identifier }
		}}
	};
	
	

	//dpl::StringStream src{ "while zero 0 do { 1 -> x; ++x; }" };
	//dpl::FileStream src{ "snippets/example.lang" };
	//dpl::FileStream src{ "snippets/short.lang" };
	//dpl::StringStream src{ "Int + ( Int + Int ; ) ;" };
	//dpl::StringStream src{ "Int + ( Int + Int )" };
	//dpl::StringStream src{ "var1 * var2 * ( var3 * var4 ) + var5 * ( var6 + (var7 + var8) ) + var9 * var10 + var11" };
	dpl::StringStream src{ "var1 + var2" };
//	dpl::StringStream src{
//R"raw(
//PROGRAM DEMO1
//BEGIN
//	A := 3;
//	B := A;
//	BABOON := GIRAFFE;
//	TEXT := "Hello, World!";
//END
//)raw"
//	};
	//dpl::StringStream src{ "PROGRAM MyProg BEGIN + + + + + + END" };

	dpl::LR1 parser{ precedence_grammar };
	//parser.printParseTable();
	

	dpl::ParseTree tree = parser.parse(src);
	
	std::cout << "\n\nGrammar:\n==============\n";
	std::cout << precedence_grammar << "\n";

	std::cout << "Input String:\n=============\n" << src.getString() << "\n";
	std::cout << tree;
	DplLogPrintTelemetry();
	return 0;
}

#undef SYMBOLS_MACRO
#undef KEYWORDS_MACRO