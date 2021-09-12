#include <iostream>
#include <algorithm>

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
	X(WideArrow, "=>")

#define KEYWORDS_MACRO \
	Y(Int) \
	Y(Return) \
	Y(Bitfield) \
	Y(True) \
	Y(False) \
	Y(If) \
	Y(Else) \
	Y(While)

#include "Tokenizer.h"

// search this solution for "#TASK" to find places where optimizations/refactoring/improvements may be worth implementing

int main() {

	const std::string test_code = "int main(int argc, char **argv) {\n\t std::cout << \"Num Of Args:\" << argc << \"\\n\";\n\t return -3.14159;\n }";
	const std::string string_test = "coming up is... a string! \"Inside the string yay\"";

	dpl::TokenizerInterface tokenizer;
	tokenizer.tokenizeString(test_code);
	std::for_each(tokenizer.base.tokens_out.begin(), tokenizer.base.tokens_out.end(), [](const auto& s) { std::cout << s << "\n"; });

	return 0;
}