#pragma once

#include "Automata.h"
#include "Token.h"
#include "LL1.h"

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

#include <magic_enum/magic_enum.hpp>

#ifdef DPL_LOG
#include <iostream>
#endif //DPL_LOG

namespace dpl {
	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class Tokenizer {
	private:

		using Token = Token<KwdT, SymT>;
		using LL1 = LL1<KwdT, SymT>;

		// #TASK : take hiders and whitespaces as input from user
		std::array<std::array<dpl::LinearDFA, 2>, 2> hiders{{ {"//", "\n"}, {"/*", "*/"} }};
		const std::unordered_set<char> whitespaces{ ' ', '\n', '\t', '\0' };
		int inside_hider = -1;
		std::string hiders_queue = "  ";

		const size_t symbols_count = magic_enum::enum_count<SymT>();

		dpl::IdentifierDFA identifier_automaton;
		dpl::NumberDFA number_automaton;
		dpl::StringDFA string_automaton;
		std::array<LinearDFA, magic_enum::enum_count<SymT>()> symbols_automata;

		std::array<GenericDFA*, magic_enum::enum_count<SymT>() + 3>
			automata { &identifier_automaton, &number_automaton, &string_automaton };
		
		const size_t misc_automata_count = automata.size() - symbols_count;

		std::unordered_map<std::string_view, KwdT> keywords;

		int longest_accepted = -1, length_of_longest = 0;
		std::string lexeme_buff = "", lexer_queue = "";

		#ifdef DPL_LOG
		std::chrono::time_point<std::chrono::steady_clock> frontend_clock_begin;
		std::chrono::time_point<std::chrono::steady_clock> char_clock_begin;
		std::chrono::duration<long double> char_avg_dur{ 0 };

		unsigned long long char_count{ 0 };
		unsigned long long token_count{ 0 };

		std::uintmax_t source_size;
		#endif //DPL_LOG

		std::pair<unsigned int, unsigned int> pos_in_file{ 0, 0 };
		std::string_view current_line;

		bool active = true;
		LL1& out_parser;

		void tokenizeLine(std::string_view src) {
			current_line = src;
			for (const auto& c : src) {
				*this << c;

				if (!this->active) break;

				pos_in_file.first++;
			}
			*this << '\n';
		}

	public:

		constexpr Tokenizer(const std::unordered_map<std::string_view, KwdT>& keywords_,
							const std::vector<std::string_view>& symbols,
							LL1& op) : keywords(keywords_), out_parser(op) {
			std::transform(symbols.begin(), symbols.end(), symbols_automata.begin(), std::identity{});
			std::transform(symbols_automata.begin(), symbols_automata.end(), automata.begin() + 3, [](auto& elm) { return &elm; });
		}

		void tokenizeFile(std::filesystem::path src) {
			std::ifstream in{ src };

			if (in.is_open()) {
				std::string line;

				// #TASK : add some option to cut-off character stream while parsing (when encountering an error...)

				pos_in_file = { 1, 1 };

				#ifdef DPL_LOG
				frontend_clock_begin = std::chrono::steady_clock::now();

				source_size = std::filesystem::file_size(src);

				dpl::log::telemetry_info.add("source file path", std::string{ src.string() });
				dpl::log::telemetry_info.add("source file size", dpl::log::FileSize{ source_size });
				#endif //DPL_LOG

				while (std::getline(in, line)) {
					tokenizeLine(line);

					pos_in_file.first = 1;
					pos_in_file.second++;

					if (!this->active) break;
				}

				this->endStream();

				in.close();
			} else {
				std::cerr << "Could not open file!";
			}
		}

		void tokenizeString(std::string_view src) {
			#ifdef DPL_LOG
			frontend_clock_begin = std::chrono::steady_clock::now();
			#endif //DPL_LOG

			tokenizeLine(src);
			this->endStream();
		}

	private:

		void endStream() {
			out_parser << Token{ TokenType::EndOfFile };

			#ifdef DPL_LOG
			auto duration = std::chrono::steady_clock::now() - frontend_clock_begin;

			dpl::log::telemetry_info.add("total time for frontend", duration);
			dpl::log::telemetry_info.add("avg. time per character", char_avg_dur);
			dpl::log::telemetry_info.add("# of characters", char_count);
			dpl::log::telemetry_info.add("# of tokens", token_count);

			dpl::log::telemetry_info.add("Apprx. speed [MB/sec]", static_cast<long double>(source_size * 1000.0l / duration.count()));
			#endif //DPL_LOG
		}

		void operator<<(const char& c) {
			#ifdef DPL_LOG
			char_clock_begin = std::chrono::steady_clock::now();
			char_count++;
			#endif
			passHiders(c);
			#ifdef DPL_LOG
			char_avg_dur *= char_count - 1;
			char_avg_dur += std::chrono::steady_clock::now() - char_clock_begin;
			char_avg_dur /= char_count;
			#endif
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

			static bool printed_error = false;

			Token tkn;

			if (machine == -1) {
				#ifdef DPL_LOG
				token_count--;
				#endif //DPL_LOG

				std::cerr << "Illegal token at character " << pos_in_file.first << " of line " << pos_in_file.second;

			} else if (machine == 0) {
				if (keywords.contains(str)) {
					tkn = Token{ TokenType::Keyword, keywords[str] };
				} else {
					tkn = Token{ TokenType::Identifier, std::string{ str } };
				}
			} else if (machine == 1) {
				long double dbl;
				std::from_chars(str.data(), str.data() + str.size(), dbl);
				tkn = Token{ TokenType::Number, dbl };
			} else if (machine == 2) {
				tkn = Token{ TokenType::String, std::move(dpl::StringDFA::recent_string) };
			} else if (machine <= symbols_count - 1 + misc_automata_count) {
				tkn = Token{ TokenType::Symbol, magic_enum::enum_value<SymT>(machine - misc_automata_count) };
			}

			tkn.pos = { pos_in_file.first - length_of_longest, pos_in_file.second };

			try {
				out_parser << tkn;
			} catch (const std::bad_function_call&) {
				if (!printed_error) {
					std::cerr << "No output callback set for tokenizer";
					printed_error = true;
				}
			}
		}

	};
}