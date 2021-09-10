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
		int inside_hider = -1;
		std::string hiders_queue = "";
		// #TASK : should probably implement custom queue with size max(hiders.first...)-1, save tons of allocations

		// #TASK : constexpr hash tables
		const std::unordered_set<std::string> keywords{ "do", "double", "u", "b", "l" };
		const std::unordered_set<char> whitespaces{ ' ', '\n', '\t' };

		std::array<dpl::LinearDFA, 5> keyword_nfas;

		int length_of_longest = 0;
		std::string lexeme_buff = "", backtrack_bank = "";

		std::list<std::string> tokens_out;

		Tokenizer() {
			hiders_states.fill(0);

			int i = 0;
			for (const auto& kw : keywords) {
				keyword_nfas[i] = { kw };
				i++;
			}
		}

		void operator<<(char c) {
			passHiders(c);
		}

		void passHiders(char c) {
			if (inside_hider == -1) {
				int possible_hiders = 0;
				for (int i = 0; i < hiders.size(); i++) {
					if (c == hiders[i].first.at( hiders_states[i] )) {
						hiders_states[i]++;
						possible_hiders++;
					}

					if (hiders_states[i] == hiders[i].first.size()) {
						inside_hider = i;
						hiders_states.fill(0);
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
				if (c == hiders[inside_hider].second.at( hiders_states[inside_hider] )) {
					hiders_states[inside_hider]++;
				}
				
				if (hiders_states[inside_hider] == hiders[inside_hider].second.size()) {
					hiders_states[inside_hider] = 0;
					inside_hider = -1;
					return;
				}
			}
		}

		void bufferedNextLetter(char c) {
			backtrack_bank.push_back(c);

			do {
				char next_c = backtrack_bank[0];
				backtrack_bank.erase(0, 1);
				nextLetter(next_c);

			} while (!backtrack_bank.empty());
		}

		void nextLetter(char c) {
			int alive_words = 0;
			
			for (int i = 0; i < keyword_nfas.size(); i++) {
				if (lexeme_buff.empty()) keyword_nfas[i].setAlive();
				keyword_nfas[i].step(c);

				if (keyword_nfas[i].isAccepted()) {
					length_of_longest = keyword_nfas[i].age;
				}
				if (keyword_nfas[i].isAlive()) alive_words++;
			}

			lexeme_buff.push_back(c);

			if (alive_words == 0) {

				// #TASK : if all died but no-one accpeted, pass line number & position to an error logger and halt lexing.
				// #TASK : this entire loop should do less string manips and use more fitting structures

				tokens_out.push_back(lexeme_buff.substr(0, length_of_longest));
				lexeme_buff.erase(0, length_of_longest);

				backtrack_bank = lexeme_buff + backtrack_bank;
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