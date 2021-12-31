#pragma once

#include "Automata.h"
#include "Token.h"
#include "../TextStream.h"
#include "../Lexical.h"

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
	template<typename AtomT = char, typename TokenT = dpl::Token<>>
	class Tokenizer {
	public:

		using atom_type = AtomT;
		using token_type = TokenT;

	private:

		int longest_accepted = -1, length_of_longest = 0;
		file_pos_t offset_in_file = 0;

		const dpl::Lexicon<atom_type, token_type>& lexicon;

	public:

		Tokenizer(const Lexicon<atom_type, token_type>& g) : lexicon(g) { }

		template<dpl::initer_of_type<atom_type> IterT>
		void tokenize(IterT first, IterT last, std::function<void(token_type)> output_func) {
			offset_in_file = 0;

			auto iter = first;
			while (iter != last) {
				size_t longest_index = -1;
				decltype(iter) longest_iter = iter;

				for (int i = 0; i < lexicon.size(); i++) {
					auto result = lexicon[i].regex(iter, last);
					if (result) {
						if (std::distance(iter, *result) > std::distance(iter, longest_iter)) {
							longest_iter = *result;
							longest_index = i;
						}
					}
				}

				// throw error if the tokenizer is stuck in the middle of the input
				if (longest_index == -1)
					throw std::exception{ "unable to tokenize file at pos blah blah ..." };
				else if (longest_iter == iter)
					throw std::exception{ "lexeme of length zero not allowed" };
				else {
					output_func(lexicon[longest_index].eval(longest_index, { iter, longest_iter }));
					iter = longest_iter;
				}
			}

			// some parsers rely on receiving an EOF token at the end of the parse stream
			output_func(token_type::eof());
		}

		template<dpl::initer_of_type<atom_type> IterT>
		auto tokenize(IterT first, IterT last) {
			std::vector<token_type> output;

			tokenize(first, last, [&](auto tkn) {
				output.push_back(tkn);
			});

			return output;
		}

	};
}