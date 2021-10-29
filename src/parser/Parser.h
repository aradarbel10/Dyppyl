#pragma once

#include <string_view>

namespace dpl{
	class Parser {
	public:
		
		Parser(Grammar& g, Tokenizer& tk) : grammar(g), input(tk) { }
		ParseTree parse(dpl::TextStream& src) {
			while (!src.closed()) {
				(*this) << this->input.fetchNext();
			}

			return std::move(out_tree);
		}

	protected:

		virtual TreeBuilder& tree_builder() = 0;
		virtual void operator<<(const Token&) = 0;

		Tokenizer& input;
		Grammar& grammar;
		ParseTree out_tree;

	};

	template<class P> requires std::derived_from<P, Parser>
	const char* getParserName = "Parser";


}