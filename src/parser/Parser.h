#pragma once

#include <string_view>

namespace dpl{
	class Parser {
	public:
		
		struct Options {
			unsigned char
				  output_step_by_step	: 1 = true
				, output_tree			: 1 = true
				, output_errors			: 1 = true
				, output_tokenizer		: 1 = true
				, output_parse_table	: 1 = false
				, output_grammar		: 1 = false
				, output_grammar_info	: 1 = false
				, output_automaton		: 1 = false;

			enum class ErrorMode { Ignore, StopAtFirst, RecoverOnFollow, RepairOnFollow } error_mode = ErrorMode::Ignore;

			enum class LogDest { Console, Text, Html } dest = LogDest::Console;
			std::filesystem::path dest_dir;
		};

		Parser(Grammar& g, const Options& ops) : grammar(g), tokenizer(grammar), options(ops) { }

		ParseTree parse(dpl::TextStream& src) {
			parse_init();

			tokenizer.tokenize(src, [this](const Token& tkn) {
				*this << tkn;
			});

			return std::move(out_tree);
		}

		Options options;

	protected:

		virtual TreeBuilder& tree_builder() = 0;
		virtual void operator<<(const Token&) = 0;
		virtual void parse_init() = 0;

		Grammar& grammar;
		ParseTree out_tree;
		Tokenizer tokenizer;

	};
}