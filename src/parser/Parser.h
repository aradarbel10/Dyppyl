#pragma once

#include <string_view>

namespace dpl{
	class Parser {
	public:
		
		Parser(Grammar& g) : grammar(g) { }

		ParseTree parse(dpl::TextStream& src) {
			Tokenizer input{ src, grammar };
			parse_init();

			while (!src.closed()) {
				(*this) << input.fetchNext();
			}

			return std::move(out_tree);
		}

	protected:

		virtual TreeBuilder& tree_builder() = 0;
		virtual void operator<<(const Token&) = 0;
		virtual void parse_init() = 0;

		Grammar& grammar;
		ParseTree out_tree;

	};

	template<class P> requires std::derived_from<P, Parser>
	const char* getParserName = "Parser";


}