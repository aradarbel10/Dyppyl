#pragma once

#include "Automata.h"
#include "Token.h"
#include "../TextStream.h"

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

		const size_t symbols_count = Token::symbols.size();

		dpl::IdentifierDFA identifier_automaton;
		dpl::NumberDFA number_automaton;
		dpl::StringDFA string_automaton;
		std::vector<LinearDFA> symbols_automata;

		std::vector<GenericDFA*> automata { &identifier_automaton, &number_automaton, &string_automaton };
		
		const size_t misc_automata_count = automata.size();

		int longest_accepted = -1, length_of_longest = 0;
		std::string lexeme_buff, lexer_queue;

		#ifdef DPL_LOG
		std::chrono::time_point<std::chrono::steady_clock> frontend_clock_begin;
		std::chrono::time_point<std::chrono::steady_clock> char_clock_begin;
		std::chrono::duration<long double> char_avg_dur{ 0 };

		unsigned long long char_count{ 0 };
		unsigned long long token_count{ 0 };

		std::uintmax_t source_size;
		#endif //DPL_LOG

		std::pair<unsigned int, unsigned int> pos_in_file{ 1, 1 };
		std::string_view current_line;

		TextStream& input;
		Token next_tkn;
		bool next_tkn_ready = false;

	public:

		Tokenizer(TextStream& inp) : input(inp) {
			symbols_automata.reserve(Token::symbols.size());
			automata.reserve(automata.size() + Token::symbols.size());

			for (const auto& [key, val] : Token::symbols) {
				symbols_automata.push_back(key.data());
				automata.push_back(&*std::prev(symbols_automata.end()));
			}
		}

		Token fetchNext() {
			next_tkn_ready = false;

			while (!next_tkn_ready) {
				*this << input.fetchNext();
			}

			return next_tkn;
		}

		bool closed() {
			return input.closed();
		}

	private:

		void operator<<(const char& c) {
			#ifdef DPL_LOG
			char_clock_begin = std::chrono::steady_clock::now();
			char_count++;
			#endif
			if (c == '\n') {
				pos_in_file.first = 1;
				pos_in_file.second++;
			} else pos_in_file.first++;

			if (c == '\0') endStream();
			else passHiders(c);
			#ifdef DPL_LOG
			char_avg_dur *= char_count - 1;
			char_avg_dur += std::chrono::steady_clock::now() - char_clock_begin;
			char_avg_dur /= char_count;
			#endif
		}

		void endStream() {
			next_tkn = Token{ Token::Type::EndOfFile };
			next_tkn_ready = true;

			#ifdef DPL_LOG
			auto duration = std::chrono::steady_clock::now() - frontend_clock_begin;

			dpl::log::telemetry_info.add("total time for frontend", duration);
			dpl::log::telemetry_info.add("avg. time per character", char_avg_dur);
			dpl::log::telemetry_info.add("# of characters", char_count);
			dpl::log::telemetry_info.add("# of tokens", token_count);

			dpl::log::telemetry_info.add("Apprx. speed [MB/sec]", static_cast<long double>(source_size * 1000.0l / duration.count()));
			#endif //DPL_LOG
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
						break;
					}
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

		void evaluate(int machine, std::string_view str) {
			#ifdef DPL_LOG
			token_count++;
			#endif //DPL_LOG

			if (machine == -1) {
				#ifdef DPL_LOG
				token_count--;
				#endif //DPL_LOG

				std::cerr << "Illegal token at character " << pos_in_file.first << " of line " << pos_in_file.second;

			} else if (machine == 0) {
				if (Terminal::keywords.contains(str)) {
					next_tkn = Token{ Token::Type::Keyword, Token::keywords[str] };
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
				next_tkn = Token{ Token::Type::Symbol, Token::symbols[name]};
			} else {
				std::cerr << "Error: Programmar is an idiot!\n";
			}

			next_tkn.pos = { pos_in_file.first, pos_in_file.second };
			next_tkn_ready = true;
		}

	};
}