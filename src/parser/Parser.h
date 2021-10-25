#pragma once

#include <string_view>

namespace dpl{
	class Parser {
	public:
		
		Parser(Grammar& g, Tokenizer& tk) : grammar(g), input(tk) { }

	protected:

		virtual TreeBuilder& tree_builder() = 0;

		Tokenizer& input;
		Grammar& grammar;

	};

	template<class P> requires std::derived_from<P, Parser>
	const char* getParserName = "Parser";


}