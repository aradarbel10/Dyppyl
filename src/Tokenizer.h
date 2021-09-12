#pragma once

#include "Automata.h"

#include <utility>
#include <variant>
#include <string>

#include <unordered_set>
#include <array>

#include <filesystem>
#include <fstream>
#include <iostream>

#include <magic_enum/magic_enum.hpp>

#ifndef SYMBOLS_MACRO
#define SYMBOLS_MACRO
#endif

#ifndef KEYWORDS_MACRO
#define KEYWORDS_MACRO
#endif

namespace dpl {

	template<typename KwdT, typename SymT>
	struct IToken {
		enum class Type { Identifier, Number, String, Symbol, SoftSymbol, Keyword, Whitespace, Unknown };

		Type type;
		std::variant<std::string, double, SymT, KwdT> value;
	};

	template<typename KwdT, typename SymT>
	std::ostream& operator<<(std::ostream& os, const IToken<KwdT, SymT>& t) {
		const auto* sval = std::get_if<std::string>(&t.value);
		const auto* dval = std::get_if<double>(&t.value);
		const auto* smval = std::get_if<SymT>(&t.value);
		const auto* kwval = std::get_if<KwdT>(&t.value);

		os << "[" << magic_enum::enum_name(t.type) << ", ";

		if (sval) os << *sval;
		if (dval) os << *dval;
		if (smval) os << magic_enum::enum_name(*smval);
		if (kwval) os << magic_enum::enum_name(*kwval);

		os << "]";
		return os;
	}

	class Tokenizer {
	public:

		// stage 1 - hiders
		std::array<std::array<dpl::LinearDFA, 2>, 2> hiders{{ {"//", "\n"}, {"/*", "*/"} }};
		int inside_hider = -1;
		std::string hiders_queue = "";
		// #TASK : should probably implement custom queue with size max(hiders.first...)-1, save tons of allocations

		#define X(name, symbol) name,
		enum class Symbols {
			SYMBOLS_MACRO
		};
		#undef X

		#define Y(name) name,
		enum class Keywords { KEYWORDS_MACRO };
		#undef Y

		using Token = IToken<Keywords, Symbols>;

		const int symbols_count = magic_enum::enum_count<Symbols>();
		const int keywords_count = magic_enum::enum_count<Keywords>();

		#define X(name, symbol) symbol##_ldfa,
		#define Y(name) #name##_ldfa,
		const std::array<std::unique_ptr<dpl::GenericDFA>, 73> automata{
			std::make_unique<dpl::IdentifierDFA>(),
			std::make_unique<dpl::NumberDFA>(),
			std::make_unique<dpl::StringDFA>(),
			SYMBOLS_MACRO
			KEYWORDS_MACRO
		};
		#undef X
		#undef Y

		const int misc_automata_count = automata.size() - symbols_count - keywords_count;

		const std::unordered_set<char> whitespaces{ ' ', '\n', '\t', '\0' };
		// #TASK : constexpr hash tables

		// #TASK : use more appropriate data structure to hold queues
		int longest_accepted = -1, length_of_longest = 0;
		std::string lexeme_buff = "", lexer_queue = "";

		std::list<Token> tokens_out;

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

					std::cout << tokens_out.back() << '\n';
				}

				lexeme_buff.clear();
				longest_accepted = -1;
				length_of_longest = 0;
			}

		}

		void evaluate(int machine, const std::string& str) {
			if (machine == -1) {
				tokens_out.emplace_back(Token{ Token::Type::Unknown, str });
			} else if (machine == 0) {
				tokens_out.emplace_back(Token{ Token::Type::Identifier, str });
			} else if (machine == 1) {
				tokens_out.emplace_back(Token{ Token::Type::Number, std::stod(str) });
			} else if (machine == 2) {
				tokens_out.emplace_back(Token{ Token::Type::String, std::move(dpl::StringDFA::recent_string) });
			} else if (machine <= symbols_count - 1 + misc_automata_count) {
				tokens_out.emplace_back(Token{ Token::Type::Symbol, magic_enum::enum_value<Symbols>(machine - misc_automata_count) });
			} else if (machine <= keywords_count - 1 + misc_automata_count + symbols_count) {
				tokens_out.emplace_back(Token{ Token::Type::Keyword, magic_enum::enum_value<Keywords>(machine - misc_automata_count - symbols_count) });
			}
		}

	};

	class TokenizerInterface {
	public:

		void tokenizeString(std::string_view src) {
			for (const auto& c : src) {
				base << c;
			}
			base << '\n';
		}

		// #TASK : tokenize files in terms of tokenizeString (do it line by line)
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

		Tokenizer base;

	};
}

#undef SYMBOLS_MACRO
#undef KEYWORDS_MACRO