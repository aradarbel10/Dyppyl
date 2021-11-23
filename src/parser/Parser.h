#pragma once

#include <string_view>

namespace dpl{
	class Parser {
	public:
		
		Parser(Grammar& g) : grammar(g), tokenizer(grammar) { }

		ParseTree parse(dpl::TextStream& src) {
			parse_init();

			tokenizer.tokenize(src, [this](const Token& tkn) { *this << tkn; });

			return std::move(out_tree);
		}

	protected:

		virtual TreeBuilder& tree_builder() = 0;
		virtual void operator<<(const Token&) = 0;
		virtual void parse_init() = 0;

		Grammar& grammar;
		ParseTree out_tree;
		Tokenizer tokenizer;

	};

	template<class P> requires std::derived_from<P, Parser>
	const char* getParserName = "Parser";


}