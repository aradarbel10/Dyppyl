#pragma once

#include "Token.h"
#include "Grammar.h"

#include <iostream>
#include <windows.h>

#include <array>
#include <variant>
#include <optional>

namespace dpl {

	template<typename KwdT, typename SymT>
	class ParseTree {
	public:

		using Token = Token<KwdT, SymT>;
		using Node = std::variant<Token, std::pair<std::string_view, int>, std::monostate>;
		using Grammar = Grammar<KwdT, SymT>;

		constexpr ParseTree() { }

		constexpr ParseTree(Grammar& g_) : grammar(g_) { }
		constexpr ParseTree(Grammar& g_, Node n_) : grammar(g_), value(n_) { }

		constexpr bool operator<<(std::variant<Token, std::pair<std::string_view, int>> node) {
			if (!value.has_value()) {
				std::visit([&](const auto& v) { value.emplace(v); }, node);

				if (auto* const pair_val = std::get_if<std::pair<std::string_view, int>>(&value.value())) {
					const auto& rule = grammar[(*pair_val).first][(*pair_val).second];

					if (rule.size() == 1 && std::holds_alternative<std::monostate>(rule[0])) {
						children.reserve(1);
						children.push_back(std::make_unique<ParseTree>(grammar, std::monostate()));
					} else {
						children.reserve(rule.size());
						for (int i = 0; i < children.capacity(); i++) {
							children.push_back(std::make_unique<ParseTree>(grammar));
						}
					}
				}
				
				return true;
			}

			for (auto& child : children) {
				if (*child << node) return true;
			}

			return false;
		}

		template<typename KwdT, typename SymT>
		friend void printTree(const std::string& prefix, ParseTree<KwdT, SymT>* node, bool isLast);

	private:

		Grammar& grammar;

		std::optional<Node> value;
		std::vector<std::unique_ptr<ParseTree>> children;
		
	};

	template<typename KwdT, typename SymT>
	void printTree(const std::string& prefix, ParseTree<KwdT, SymT>* node, bool isLast) {
		static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		std::cout << prefix;
		std::cout << (isLast ? "'---" : "|---");

		if (node->value.has_value()) {
			if (const auto* nt = std::get_if<std::pair<std::string_view, int>>(&node->value.value())) std::cout << (*nt).first << "(" << (*nt).second << ")\n";
			else if (const auto* tkn = std::get_if<Token<KwdT, SymT>>(&node->value.value())) {
				SetConsoleTextAttribute(hConsole, 0x03);
				std::cout << (*tkn).stringify() << '\n';
				SetConsoleTextAttribute(hConsole, 0x07);
			} else {
				std::cout << "null\n";
			}
		}
		

		for (int i = 0; i < node->children.size(); i++) {
			printTree(prefix + (isLast ? "    " : "|   "), &*node->children[i], i == node->children.size() - 1);
		}
	}

	template<typename KwdT, typename SymT>
	void printTree(ParseTree<KwdT, SymT>& node) {
		std::cout << "\n\nParse Tree:\n========================\n";
		printTree("", &node, true);
		std::cout << "\n\n";
	}

}