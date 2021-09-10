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

		// #TASK : initialize NFAs directly with all the strings
		const std::array<std::pair<std::string, std::string>, 2> hiders{{ {"//", "\n"}, {"/*", "*/"} }};
		std::array<std::array<dpl::LinearDFA, 2>, 2> hider_nfas;
		int inside_hider = -1;
		std::string hiders_queue = "";
		// #TASK : should probably implement custom queue with size max(hiders.first...)-1, save tons of allocations

		// #TASK : constexpr hash tables
		const std::array<std::string, 5> keywords{ "do", "double", "u", "b", "l" };
		const std::unordered_set<char> whitespaces{ ' ', '\n', '\t', '\0'};

		std::array<dpl::LinearDFA, 5> keyword_nfas;

		int length_of_longest = 0;
		std::string lexeme_buff = "", lexer_queue = "";

		std::list<Token> tokens_out;

		Tokenizer() {
			for (int i = 0; i < keywords.size(); i++) {
				keyword_nfas[i] = { keywords[i] };
			}
			
			for (int i = 0; i < hiders.size(); i++) {
				hider_nfas[0][i] = { hiders[i].first };
				hider_nfas[1][i] = { hiders[i].second };
			}
		}

		void operator<<(char c) {
			passHiders(c);
		}

		void passHiders(char c) {
			if (inside_hider == -1) {
				int possible_hiders = 0;
				for (int i = 0; i < hiders.size(); i++) {
					if (hiders_queue.empty()) hider_nfas[0][i].setAlive();
					hider_nfas[0][i].step(c);

					if (hider_nfas[0][i].isAlive()) possible_hiders++;

					if (hider_nfas[0][i].isAccepted()) {
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
				if(!hider_nfas[1][inside_hider].isAlive()) hider_nfas[1][inside_hider].setAlive();
				hider_nfas[1][inside_hider].step(c);

				if (hider_nfas[1][inside_hider].isAccepted()) {
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

			for (int i = 0; i < keyword_nfas.size(); i++) {
				if (lexeme_buff.empty()) keyword_nfas[i].setAlive();
				keyword_nfas[i].step(c);

				if (keyword_nfas[i].isAccepted()) {
					length_of_longest = keyword_nfas[i].age;
					accepted_nfa = i;
				}
				if (keyword_nfas[i].isAlive()) alive_words++;
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