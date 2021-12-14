#pragma once

#include "Automata.h"
#include "Token.h"
#include "../TextStream.h"
#include "../Grammar.h"

#include <utility>
#include <variant>
#include <string>
#include <iterator>
#include <functional>
#include <charconv>

#include <unordered_set>
#include <array>
#include <queue>

#include <filesystem>
#include <fstream>
#include <cstdint>

#ifdef DPL_LOG
#include <iostream>
#endif //DPL_LOG

namespace dpl {
	class Tokenizer {
	private:

		// #TASK : take hiders and whitespaces as input from user
		std::array<std::array<dpl::LinearDFA, 2>, 2> hiders{{ {"//", "\n"}, {"/*", "*/"} }};
		const std::unordered_set<char> whitespaces{ ' ', '\n', '\t', '\0' };
		int inside_hider = -1;
		std::string hiders_queue;

		size_t symbols_count;

		dpl::IdentifierDFA identifier_automaton;
		dpl::NumberDFA number_automaton;
		dpl::StringDFA string_automaton;
		std::vector<LinearDFA> symbols_automata;

		std::vector<GenericDFA*> automata { &identifier_automaton, &number_automaton, &string_automaton };
		
		const size_t misc_automata_count = automata.size();

		int longest_accepted = -1, length_of_longest = 0;
		std::string lexeme_buff, lexer_queue;

		file_pos_t pos_in_file, start_of_lexeme;

		Grammar& grammar;
		std::function<void(Token)> output_func;

	public:

		Tokenizer(Grammar& g) : grammar(g) {
			symbols_count = grammar.symbols.size();

			symbols_automata.reserve(grammar.symbols.size());
			automata.reserve(automata.size() + grammar.symbols.size());

			for (const auto& key : grammar.symbols) {
				symbols_automata.push_back(key.data());
				automata.push_back(&*std::prev(symbols_automata.end()));
			}
		}

		void tokenize(TextStream& input, std::function<void(Token)> out_f) {
			pos_in_file = { 0, 0 };
			output_func = out_f;

			while (!input.closed()) {
				*this << input.fetchNext();
			}
		}

	private:

		void operator<<(const char& c) {
			pos_in_file.offset++;

			if (c == '\n') {
				pos_in_file.col = 1;
				pos_in_file.row++;
			} else pos_in_file.col++;

			if (c == '\0') endStream();
			else passHiders(c);
		}

		void endStream() {
			Token eof_tkn{ Token::Type::EndOfFile };
			eof_tkn.pos = start_of_lexeme;
			output_func(eof_tkn);
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
					if (i < misc_automata_count) {
						all_dead = true;
						//break;
					}
				}
			}

			if (lexeme_buff.empty()) start_of_lexeme = { pos_in_file.row, pos_in_file.col - 1, pos_in_file.offset };
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

		void evaluate(int machine, std::string_view str) {
			Token next_tkn;

			if (machine == -1) {

				std::cerr << "Illegal token at character " << start_of_lexeme.col << " of line " << start_of_lexeme.row;

			} else if (machine == 0) {
				auto iter = std::find(grammar.keywords.begin(), grammar.keywords.end(), str);
				if (iter != grammar.keywords.end()) {
					next_tkn = Terminal{ Token::Type::Keyword, std::string_view{ *iter } };
				} else {
					next_tkn = Token{ Token::Type::Identifier, std::string{ str } };
				}
			} else if (machine == 1) {
				long double dbl;
				std::from_chars(str.data(), str.data() + str.size(), dbl);
				next_tkn = Token{ Token::Type::Number, dbl };
			} else if (machine == 2) {
				next_tkn = Token{ Token::Type::String, std::move(dpl::StringDFA::recent_string) };
			} else if (machine <= symbols_count - 1 + misc_automata_count) {
				std::string_view name = static_cast<LinearDFA*>(automata[machine])->getStates();
				auto iter = std::find(grammar.symbols.begin(), grammar.symbols.end(), name);
				next_tkn = Terminal{ Token::Type::Symbol, name };
			} else {
				std::cerr << "Error: Programmar is an idiot!\n";
			}

			next_tkn.pos = start_of_lexeme;

			output_func(next_tkn);
		}

	};
}