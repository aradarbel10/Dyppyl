#pragma once

#include <utility>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <variant>
#include <iostream>
#include <fstream>

namespace dpl {

	struct Token {
		enum class Type { Identifier, Number, String, Symbol, Keyword };

		const Type type;
		const std::variant<std::string, double> value;
	};

	std::ostream& operator<<(std::ostream& os, const Token& t) {
		const auto* sval = std::get_if<std::string>(&t.value);
		const auto* dval = std::get_if<double>(&t.value);

		os << "[" << static_cast<int>(t.type) << ", ";

		if (sval) os << *sval;
		else if (dval) os << *dval;

		os << "]";
		return os;
	}

	class Tokenizer {
	private:

		const std::unordered_multimap<std::string, std::string> hiders{ {"//", "\n"}, {"/*", "*/"} };
		const std::unordered_set<std::string> keywords{ "void", "int", "true", "false", "if", "while" };
		const std::unordered_set<std::string> symbols{ "+", "-", "*", "/", "%", "(", ")" };

		std::string lexeme_buff;

	public:

		void nextToken(char c) {
			static bool taking_symbols = false;

			static std::string in_hider = "";

			lexeme_buff.push_back(c);

			if (in_hider.empty()) {
				if (hiders.contains(lexeme_buff)) {
					in_hider = lexeme_buff;
				} else {
					//lex tokens

					std::cout << c;

				}
			} else {
				//handle escaping from hider
			}
		}

		constexpr double parseNumber(std::string_view num) {
			return 0;
		}
	};

	class TokenizerInterface {
	public:

		void tokenizeFile(std::filesystem::path src) {
			std::ifstream in(src);

			if (in.is_open()) {
				char c;
				while (in >> std::noskipws >> c) {
					t.nextToken(c);
				}
			} else {
				throw "Could not open file!";
			}
		}

		void tokenizeString(std::string_view src) {
			for (const auto& c : src) {
				t.nextToken(c);
			}
		}

	private:

		Tokenizer t;

	};
}