#pragma once

#include "Token.h"
#include "Grammar.h"

#include <iostream>
#include <windows.h>

#include <array>
#include <variant>

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
			if (std::holds_alternative<std::monostate>(value)) {
				std::visit([&](const auto& v) { value = v; }, node);

				if (auto* const pair_val = std::get_if<std::pair<std::string_view, int>>(&value)) {
					children.reserve(grammar[(*pair_val).first][(*pair_val).second].size());
					for (int i = 0; i < children.capacity(); i++) {
						//if (const auto* terminal = std::get_if<Token>(&grammar[(*pair_val).first][(*pair_val).second][i])) {
						//	children.emplace_back(std::make_unique<ParseTree>(grammar, *terminal));
						//} else {
						//	
						//}
						children.push_back(std::make_unique<ParseTree>(grammar));
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

		Node value = std::monostate();
		std::vector<std::unique_ptr<ParseTree>> children;
		
	};

	template<typename KwdT, typename SymT>
	void printTree(const std::string& prefix, ParseTree<KwdT, SymT>* node, bool isLast) {
		static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		std::cout << prefix;
		std::cout << (isLast ? "'---" : "|---");

		//std::cout << node->value.first << " (" << node->value.second << ")\n";
		if (const auto* nt = std::get_if<std::pair<std::string_view, int>>(&node->value)) std::cout << (*nt).first << "(" << (*nt).second << ")\n";
		else if (const auto* tkn = std::get_if<Token<KwdT, SymT>>(&node->value)) {
			SetConsoleTextAttribute(hConsole, 0x03);
			std::cout << (*tkn).stringify() << '\n';
			SetConsoleTextAttribute(hConsole, 0x07);
		}

		for (int i = 0; i < node->children.size(); i++) {
			printTree(prefix + (isLast ? "    " : "|   "), &*node->children[i], i == node->children.size() - 1);
			//if (const auto* child_node = std::get_if<std::unique_ptr<ParseTree<KwdT, SymT>>>(&node->children[i])) {
			//	
			//} else if (const auto* leaf_node = std::get_if<Token<KwdT, SymT>>(&node->children[i])) {
			//	std::cout << prefix << (isLast ? "    " : "|   ") << (i == node->children.size() - 1 ? "'---" : "|---");

			//	SetConsoleTextAttribute(hConsole, 0x03);
			//	std::cout << (*leaf_node).stringify() << '\n';
			//	SetConsoleTextAttribute(hConsole, 0x07);
			//}
		}
	}

	template<typename KwdT, typename SymT>
	void printTree(ParseTree<KwdT, SymT>* node) {
		printTree("", node, true);
	}

}