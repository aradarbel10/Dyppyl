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

	dpl::LR1 parser{ grammar };


	dpl::StringStream src{ "(( 5 + 4.0 ) * ( 18.2 + -2 ))" };
	dpl::ParseTree tree = parser.parse(src);

	tree.replace_with(
		dpl::ParseTree{ "E", {{ "("_sym }, {}, { "Op" }, {}, {")"_sym}}},
		[](const std::vector<dpl::ParseTree>& cs) {
			return dpl::ParseTree{ cs[1][0].value, {
				(cs[0].match({"E"}) ? cs[0][0].value : cs[0]),
				(cs[2].match({"E"}) ? cs[2][0].value : cs[2])
			}};
		},
		dpl::TraverseOrder::BottomUp
	);

	std::cout << "\n\nAST:\n" << tree << "\n\n";

	dpl::tree_visit(tree, [](dpl::ParseTree& tree) {
		if (tree.children.size() == 2) {
			long double lhs = std::get<long double>(std::get<dpl::Token>(tree[0].value).value);
			long double rhs = std::get<long double>(std::get<dpl::Token>(tree[1].value).value);

			if (tree.match({"+"_sym}))
				tree = dpl::ParseTree{{ dpl::Token{dpl::Token::Type::Number, lhs + rhs }}};
			else if (tree.match({"*"_sym }))
				tree = dpl::ParseTree{{ dpl::Token{dpl::Token::Type::Number, lhs * rhs }}};
		}
	});
	
	std::cout << "Input String: " << src.getString() << "\n";
	std::cout << "Result: " << std::get<long double>(std::get<dpl::Token>(tree.value).value);

	

	return 0;
}

#undef SYMBOLS_MACRO
#undef KEYWORDS_MACRO