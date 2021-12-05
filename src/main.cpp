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

	//using Token = dpl::Token;

	dpl::Grammar grammar{
		{ "E", {
			{ "("_sym, "E", "Op", "E", ")"_sym },
			{ dpl::Token::Type::Number }
		}},
		{ "Op", {
			{ "+"_sym },
			{ "*"_sym }
		}}
	};
	grammar.symbols = { "+", "*", "(", ")" };
	
	dpl::StringStream src{ "(( 5 + 4.0 ) * ( 18.2 + -2 ))" };

	dpl::LL1 parser{ grammar };
	dpl::ParseTree tree = parser.parse(src);
	
	using dpl::RuleRef;
	dpl::TreePattern expected_tree
	{ "E", {
		{ "("_sym },
		{ "E" },
		{ "Op", {{ "+"_sym }}},
		{ "E" },
		{ ")"_sym }
	}};


	std::cout << "\n\nGrammar:\n==============\n";
	std::cout << grammar << "\n";

	std::cout << "Input String:\n=============\n" << src.getString() << "\n";
	std::cout << tree;

	return 0;
}

#undef SYMBOLS_MACRO
#undef KEYWORDS_MACRO