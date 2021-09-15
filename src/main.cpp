#include <iostream>
#include <algorithm>

#include "Tokenizer.h"
#include "Token.h"
#include "Grammar.h"
#include "LL1.h"

// search this solution for "#TASK" to find places where optimizations/refactoring/improvements may be worth implementing

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
	X(WideArrow, "=>")

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
	X(Do, "do")

int main() {

	#define X(name, symbol) name,
	#define Y(name) name,
	enum class Symbols { SYMBOLS_MACRO };
	enum class Keywords { KEYWORDS_MACRO };
	#undef X
	#undef Y

	#define X(name, symbol) symbol,
	#define Y(name) #name,
	const std::vector<std::string_view> symbols{ SYMBOLS_MACRO };
	const std::vector<std::string_view> keywords{ KEYWORDS_MACRO };
	#undef X
	#undef Y

	dpl::Tokenizer<Keywords, Symbols> tokenizer{ keywords, symbols };

	//const std::string test_code = "int main(int argc, char **argv) {\n\t std::cout << \"Num Of Args:\" << argc << \"\\n\";\n\t return -3.14159;\n }";
	//tokenizer.tokenizeString(test_code);

	tokenizer.tokenizeFile("snippets/python.py");
	std::for_each(tokenizer.tokens_out.begin(), tokenizer.tokens_out.end(), [](const auto& s) { std::cout << s << "\n"; });
	std::cout << "\n\n\n\n";


	using ProductionRule = dpl::ProductionRule<Keywords, Symbols>;
	using Nonterminal = dpl::Nonterminal<Keywords, Symbols>;
	using Token = dpl::Token<Keywords, Symbols>;
	using Special = ProductionRule::Special;
	using Grammar = dpl::Grammar<Keywords, Symbols>;
	using LL1 = dpl::LL1<Keywords, Symbols>;
	
	//Nonterminal Term{ "Term", {
	//	{ Token::Type::Identifier },
	//	{ Token::Type::Number }
	//}};

	//Nonterminal Expr{ "Expr", {
	//	{ "Term", Symbols::Arrow, Token::Type::Identifier },
	//	{ Keywords::Zero, Symbols::Question, "Term"},
	//	{ Keywords::Not, Special::Self },
	//	{ Symbols::PlusPlus, Token::Type::Identifier },
	//	{ Symbols::MinusMinus, Token::Type::Identifier }
	//}};

	//Nonterminal Stmt{ "Stmt", {
	//	{ Keywords::If, "Expr" , Keywords::Then, "Stmt" },
	//	{ Keywords::While, "Expr", "Stmt" },
	//	{ "Expr", Symbols::Semicolon }
	//}};

	Grammar grammar{
		{ "Term", {
			{ Token::Type::Identifier },
			{ Token::Type::Number }
		}},
		{ "Expr", {
			{ "Term", Symbols::Arrow, Token::Type::Identifier },
			{ Keywords::Zero, Symbols::Question, "Term"},
			{ Keywords::Not, Special::Self },
			{ Symbols::PlusPlus, Token::Type::Identifier },
			{ Symbols::MinusMinus, Token::Type::Identifier }
		}},
		{ "Stmt", {
				{ Keywords::If, "Expr" , Keywords::Then, "Stmt" },
				{ Keywords::While, "Expr", "Stmt" },
				{ "Expr", Symbols::Semicolon }
		}}
	};
	LL1 parser{ grammar };

	//std::cout << Op << '\n';
	//std::cout << E << "\n\n\n";

	return 0;
}

#undef SYMBOLS_MACRO
#undef KEYWORDS_MACRO