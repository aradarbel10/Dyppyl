#pragma once

#include "Token.h"
#include "Grammar.h"

#include <iostream>
#include <windows.h>

#include <array>
#include <variant>
#include <optional>

namespace dpl {

	class ParseTree {
	public:

		using Node = std::variant<Token, std::pair<std::string_view, int>, std::monostate>;


		ParseTree(Grammar& g_) : grammar(g_) { }
		ParseTree(Grammar& g_, Node n_) : grammar(g_), value(n_) { }


		bool operator<<(std::variant<Token, std::pair<std::string_view, int>> node) {
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

		friend std::ostream& operator<<(std::ostream& os, const ParseTree& tree) {
			os << "\n\nParse Tree:\n========================\n";
			tree.recursivePrint("", true);
			os << "\n\n";

			return os;
		}


	private:

		void recursivePrint(const std::string& prefix, bool isLast) const {
			static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

			std::cout << prefix;
			std::cout << (isLast ? "'---" : "|---");

			if (value.has_value()) {
				if (const auto* nt = std::get_if<std::pair<std::string_view, int>>(&value.value())) std::cout << (*nt).first << "(" << (*nt).second << ")\n";
				else if (const auto* tkn = std::get_if<Token>(&value.value())) {
					SetConsoleTextAttribute(hConsole, 0x03);
					std::cout << (*tkn).stringify() << '\n';
					SetConsoleTextAttribute(hConsole, 0x07);
				} else {
					std::cout << "null\n";
				}
			}


			for (int i = 0; i < children.size(); i++) {
				children[i]->recursivePrint(prefix + (isLast ? "    " : "|   "), i == children.size() - 1);
			}
		}

	private:

		Grammar& grammar;

		std::optional<Node> value;
		std::vector<std::unique_ptr<ParseTree>> children;

		friend class BottomUpTreeBuilder;
		
	};

	
	class BottomUpTreeBuilder {
	public:

		BottomUpTreeBuilder(Grammar& g_) : grammar(g_) { };

		void addSubTree(ParseTree::Node tree) {
			sub_trees.emplace_back(std::make_unique<ParseTree>(grammar, tree));
		}

		void packTree(ParseTree::Node tree) {
			std::unique_ptr<ParseTree> new_root{ std::make_unique<ParseTree>(grammar, tree) };
			std::swap(new_root->children, sub_trees);
			sub_trees.clear();
			sub_trees.emplace_back(std::move(new_root));
		}

		void assignToTree(ParseTree& tree) {
			// #TASK : confirm there is exactly one sub-tree when accessing this
			tree.children = std::move(sub_trees.front()->children);
			tree.value = sub_trees.front()->value;
		}

	private:

		std::vector<std::unique_ptr<ParseTree>> sub_trees;
		Grammar& grammar;

	};

}