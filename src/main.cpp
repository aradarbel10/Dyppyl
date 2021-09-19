#include <iostream>
#include <algorithm>
#include <memory>
#include <string_view>
#include <variant>
#include <unordered_map>

#define DPL_LOG

#include "Logger.h"

#include "Tokenizer.h"
#include "Token.h"
#include "Grammar.h"
#include "LL1.h"
#include "ParseTree.h"

// search this solution for "#TASK" to find places where optimizations/refactoring/improvements may be worth implementing

// #TASK : hook up all output to some external logger
// #TASK : abstract away tokenizer and parser inputs as buffered streams
// #TASK : use callbacks for all stream interactions
// #TASK : wrap up tokenizer, parser, tree-gen in a separate "front end" object
// #TASK : carry positional information inside tokens
// #TASK : use macros to enable/disable debugging features
// #TASK : find better way to handle exceptions
// #TASK : ensure constexpr-ness of whatever's possible
// #TASK : write tests
// #TASK : separate code to header/cpp files

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
	Y(World)

int main() {

	#define X(name, symbol) name,
	#define Y(name) name,
	enum class Symbols { SYMBOLS_MACRO };
	enum class Keywords { KEYWORDS_MACRO };
	#undef X
	#undef Y

	#define X(name, symbol) symbol,
	const std::vector<std::string_view> symbols{ SYMBOLS_MACRO };
	#undef X
	
	#define X(name, str) {str, Keywords:: name},
	#define Y(name) {#name, Keywords:: name},
	const std::unordered_map<std::string_view, Keywords> keywords{ KEYWORDS_MACRO };
	#undef X
	#undef Y

	

	using ProductionRule = dpl::ProductionRule<Keywords, Symbols>;
	using Nonterminal = dpl::Nonterminal<Keywords, Symbols>;
	using Token = dpl::Token<Keywords, Symbols>;
	using Grammar = dpl::Grammar<Keywords, Symbols>;
	using LL1 = dpl::LL1<Keywords, Symbols>;
	using ParseTree = dpl::ParseTree<Keywords, Symbols>;

	Grammar example_grammar{
		{ "Term", {
			{ Token::Type::Identifier },
			{ Token::Type::Number }
		}},
		{ "Expr", {
			{ "Term", Symbols::Arrow, Token::Type::Identifier },
			{ Keywords::Zero, "Term"},
			{ Keywords::Not, "Expr" },
			{ Symbols::PlusPlus, Token::Type::Identifier },
			{ Symbols::MinusMinus, Token::Type::Identifier }
		}},
		{ "Stmt", {
			{ Keywords::If, "Expr" , Keywords::Then, "Block" },
			{ Keywords::While, "Expr", Keywords::Do, "Block" },
			{ "Expr", Symbols::Semicolon }
		}},
		{ "Block", {
			{ "Stmt" },
			{ Symbols::LeftCurly, "Stmt", Symbols::RightCurly }
		}},
		{ "Stmts", {
			{ "Stmt", "Stmts" },
			{ std::monostate() }
		}}
	};

	LL1 parser{ example_grammar, "Stmts" };

	dpl::Tokenizer<Keywords, Symbols> tokenizer{ keywords, symbols };

	tokenizer.setOutputCallback([&parser](const Token& tkn) {
		auto res = parser << tkn;
		if (res == dpl::ParseResult::Fail) {
			std::cout << "Parsing Error Occured!\n";
		} else if (res == dpl::ParseResult::Done) {
			if (tkn.type != dpl::TokenType::EndOfFile) {
				std::cout << "Parsing Finished Too Soon...\n";
			}
		}
	});

	std::string_view input_text{ "while not zero var do --var;" };
	tokenizer.tokenizeString(input_text);


	ParseTree tree{example_grammar};


	std::for_each(parser.parse_out.begin(), parser.parse_out.end(), [&](const std::variant<Token, std::pair<std::string_view, int>>& e) {
		std::cout << dpl::log::streamer{ e } << '\n';

		if (!(tree << e)) std::cout << "Parse Tree Overloaded!";
	});

	DplLogPrintParseTree(tree);
	DplLogPrintTelemetry();
	DplLogPrintParseErrors();

	return 0;
}

#undef SYMBOLS_MACRO
#undef KEYWORDS_MACRO