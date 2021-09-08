#pragma once

#include <utility>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <variant>
#include <iostream>
#include <fstream>
#include <array>

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
	public:

		const std::vector<std::pair<std::string, std::string>> hiders{ {"//", "\n"}, {"/*", "*/"} };
		std::array<int, 2> hiders_states;
		unsigned int inside_hider = 2;

		const std::unordered_set<std::string> keywords{ "void", "int", "true", "false", "if", "while" };
		const std::unordered_set<std::string> symbols{ "+", "-", "*", "/", "%", "(", ")" };
		const std::unordered_set<char> whitespaces{ ' ', '\n', '\t' };

		std::string lexeme_buff;

	

		Tokenizer() {
			hiders_states.fill(0);
		}

		void operator<<(char c) {
			passHiders(c);
		}

		void passHiders(char c) {
			if (inside_hider == hiders.size()) { //not inside any hider
				for (int i = 0; i < hiders.size(); i++) {
					if (c == hiders[i].first.at( hiders_states[i] )) {
						hiders_states[i]++;
					}

					if (hiders_states[i] == hiders[i].first.size()) {
						inside_hider = i;
						hiders_states.fill(0);

						lexeme_buff.erase(lexeme_buff.size() - hiders[i].first.size() + 1);

						return;
					}
				}

				nextLetter(c);

			} else {
				if (c == hiders[inside_hider].second.at( hiders_states[inside_hider] )) {
					hiders_states[inside_hider]++;
				}
				
				if (hiders_states[inside_hider] == hiders[inside_hider].second.size()) {
					hiders_states[inside_hider] = 0;
					inside_hider = hiders.size();
					return;
				}
			}
		}

		void nextLetter(char c) {
			lexeme_buff.push_back(c);
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
					base << c;
				}
			} else {
				throw "Could not open file!";
			}
		}

		void tokenizeString(std::string_view src) {
			for (const auto& c : src) {
				base << c;
			}
		}

		Tokenizer base;

	};
}