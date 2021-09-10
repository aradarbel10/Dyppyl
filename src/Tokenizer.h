#pragma once

#include "LinearDFA.h"

#include <utility>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <variant>
#include <iostream>
#include <fstream>
#include <array>

#include <magic_enum/magic_enum.hpp>

namespace dpl {

	struct Token {
		enum class Type { Identifier, Number, String, Symbol, Keyword, Whitespace };

		Type type;
		std::variant<std::string, double> value;
	};

	std::ostream& operator<<(std::ostream& os, const Token& t) {
		const auto* sval = std::get_if<std::string>(&t.value);
		const auto* dval = std::get_if<double>(&t.value);

		os << "[" << magic_enum::enum_name(t.type) << ", ";

		if (sval) os << *sval;
		else if (dval) os << *dval;

		os << "]";
		return os;
	}

	class Tokenizer {
	public:

		std::array<std::array<dpl::LinearDFA, 2>, 2> hiders{{ {"//", "\n"}, {"/*", "*/"} }};
		int inside_hider = -1;
		std::string hiders_queue = "";
		// #TASK : should probably implement custom queue with size max(hiders.first...)-1, save tons of allocations

		// #TASK : constexpr hash tables
		std::array<dpl::LinearDFA, 5> keywords{ "do", "double", "u", "b", "l" };
		const std::unordered_set<char> whitespaces{ ' ', '\n', '\t', '\0'};

		int length_of_longest = 0;
		std::string lexeme_buff = "", lexer_queue = "";

		std::list<Token> tokens_out;

		Tokenizer() {
			for (int i = 0; i < keywords.size(); i++) {
				keywords[i] = { keywords[i] };
			}
		}

		void operator<<(char c) {
			passHiders(c);
		}

		void passHiders(char c) {
			if (inside_hider == -1) {
				int possible_hiders = 0;
				for (int i = 0; i < hiders.size(); i++) {
					if (hiders_queue.empty()) hiders[i][0].setAlive();
					hiders[i][0].step(c);

					if (hiders[i][0].isAlive()) possible_hiders++;

					if (hiders[i][0].isAccepted()) {
						inside_hider = i;
						hiders_queue.clear();

						bufferedNextLetter(' ');
						return;
					}
				}

				if (possible_hiders == 0) {
					while (!hiders_queue.empty()) {
						bufferedNextLetter(hiders_queue[0]);
						hiders_queue.erase(0);
					}

					bufferedNextLetter(c);
				} else {
					hiders_queue.push_back(c);
				}

			} else {
				if(!hiders[inside_hider][1].isAlive()) hiders[inside_hider][1].setAlive();
				hiders[inside_hider][1].step(c);

				if (hiders[inside_hider][1].isAccepted()) {
					inside_hider = -1;
					return;
				}
			}
		}

		void bufferedNextLetter(char c) {
			lexer_queue.push_back(c);

			do {
				char next_c = lexer_queue[0];
				lexer_queue.erase(0, 1);
				nextLetter(next_c);

			} while (!lexer_queue.empty());
		}

		void nextLetter(char c) {
			int alive_words = 0;
			int accepted_nfa = -1;

			for (int i = 0; i < keywords.size(); i++) {
				if (lexeme_buff.empty()) keywords[i].setAlive();
				keywords[i].step(c);

				if (keywords[i].isAccepted()) {
					length_of_longest = keywords[i].getAge();
					accepted_nfa = i;
				}
				if (keywords[i].isAlive()) alive_words++;
			}

			lexeme_buff.push_back(c);

			if (alive_words == 0) {

				// #TASK : if all died but no-one accpeted, pass line number & position to an error logger and halt lexing.
				// #TASK : this entire loop should do less string manips and use more fitting structures

				std::string res_token = lexeme_buff.substr(0, length_of_longest);
				
				if (res_token.size() == 1 && whitespaces.contains(res_token[0])) {
					tokens_out.push_back({ Token::Type::Whitespace, res_token });
				} else if (accepted_nfa <= 1) {
					tokens_out.push_back({ Token::Type::Keyword, res_token });
				} else {
					tokens_out.push_back({ Token::Type::Identifier, res_token });
				}

				lexeme_buff.erase(0, length_of_longest);

				lexer_queue = lexeme_buff + lexer_queue;
				lexeme_buff.clear();
			};
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