#pragma once

#include "Automata.h"
#include "Token.h"

#include <utility>
#include <variant>
#include <string>
#include <iterator>
#include <functional>

#include <unordered_set>
#include <array>
#include <queue>

#include <filesystem>
#include <fstream>
#include <iostream>

#include <magic_enum/magic_enum.hpp>

namespace dpl {
	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class Tokenizer {
	private:

		using Token = Token<KwdT, SymT>;

		std::array<std::array<dpl::LinearDFA, 2>, 2> hiders{{ {"//", "\n"}, {"/*", "*/"} }};
		const std::unordered_set<char> whitespaces{ ' ', '\n', '\t', '\0' };
		int inside_hider = -1;
		std::string hiders_queue = "";
		// #TASK : should probably implement custom queue with size max(hiders.first...)-1, save tons of allocations
		
		const size_t keywords_count = magic_enum::enum_count<KwdT>();
		const size_t symbols_count = magic_enum::enum_count<SymT>();

		dpl::IdentifierDFA identifier_automaton;
		dpl::NumberDFA number_automaton;
		dpl::StringDFA string_automaton;
		std::array<LinearDFA, magic_enum::enum_count<KwdT>()> keywords_automata;
		std::array<LinearDFA, magic_enum::enum_count<SymT>()> symbols_automata;

		std::array<GenericDFA*, magic_enum::enum_count<KwdT>() + magic_enum::enum_count<SymT>() + 3>
			automata { &identifier_automaton, &number_automaton, &string_automaton };
		
		const size_t misc_automata_count = automata.size() - symbols_count - keywords_count;

		// #TASK : find more appropriate data structure to hold queues
		int longest_accepted = -1, length_of_longest = 0;
		std::string lexeme_buff = "", lexer_queue = "";

		std::function<void(Token)> output;

	public:
		std::list<Token> tokens_out;

		constexpr Tokenizer(const std::vector<std::string_view>& keywords, const std::vector<std::string_view>& symbols) {
			std::transform(symbols.begin(), symbols.end(), symbols_automata.begin(), std::identity{});
			std::transform(keywords.begin(), keywords.end(), keywords_automata.begin(), std::identity{});
			
			std::transform(symbols_automata.begin(), symbols_automata.end(), automata.begin() + 3, [](auto& elm) { return &elm; });
			std::transform(keywords_automata.begin(), keywords_automata.end(), automata.begin() + symbols_automata.size() + 3, [](auto& elm) { return &elm; });
		}
		
		constexpr void setOutputCallback(std::function<void(Token)> fn) {
			output = fn;
		}

		void tokenizeFile(std::filesystem::path src) {
			std::ifstream in{ src };

			if (in.is_open()) {
				std::string line;

				while (std::getline(in, line)) {
					tokenizeLine(line);
				}

				this->endStream();

				in.close();
			} else {
				throw "Could not open file!";
			}
		}

		void tokenizeLine(std::string_view src) {
			for (const auto& c : src) {
				*this << c;
			}
			*this << '\n';
		}

		void tokenizeString(std::string_view src) {
			tokenizeLine(src);
			this->endStream();
		}

	private:

		void endStream() {
			//tokens_out.push_back(TokenType::EndOfFile);
			output(TokenType::EndOfFile);
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
			bool all_dead = true;

			for (int i = 0; i < automata.size(); i++) {
				if (lexeme_buff.empty()) automata[i]->setAlive();

				automata[i]->step(c);

				if (automata[i]->isAlive()) all_dead = false;
				if (automata[i]->isAccepted()) {
					longest_accepted = i;
					length_of_longest = automata[i]->getAge();
					all_dead = true;
				}
			}

			lexeme_buff.push_back(c);

			if (all_dead) {
				if (longest_accepted != -1) {
					evaluate(longest_accepted, lexeme_buff.substr(0, length_of_longest));
					if (length_of_longest != lexeme_buff.size())
						lexer_queue = lexeme_buff.substr(length_of_longest, std::string::npos) + lexer_queue;
					else automata[longest_accepted]->kill();
				}

				lexeme_buff.clear();
				longest_accepted = -1;
				length_of_longest = 0;
			}
		}

		void evaluate(int machine, const std::string& str) {
			static bool printed_error = false;

			try {
				if (machine == -1) {
					output(Token{ Token::Type::Unknown, str });
				} else if (machine == 0) {
					output(Token{ Token::Type::Identifier, str });
				} else if (machine == 1) {
					output(Token{ Token::Type::Number, std::stod(str) });
				} else if (machine == 2) {
					output(Token{ Token::Type::String, std::move(dpl::StringDFA::recent_string) });
				} else if (machine <= symbols_count - 1 + misc_automata_count) {
					output(Token{ Token::Type::Symbol, magic_enum::enum_value<SymT>(machine - misc_automata_count) });
				} else if (machine <= keywords_count - 1 + misc_automata_count + symbols_count) {
					output(Token{ Token::Type::Keyword, magic_enum::enum_value<KwdT>(machine - misc_automata_count - symbols_count) });
				}
			} catch (const std::bad_function_call& e) {
				if (!printed_error) {
					std::cout << "ERROR: no output callback set for tokenizer";
					printed_error = true;
				}
			}
		}

	};
}