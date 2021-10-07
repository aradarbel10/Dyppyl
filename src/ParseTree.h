#pragma once

#include "tokenizer/Token.h"
#include "Grammar.h"

#include <iostream>

#include <array>
#include <variant>
#include <optional>

namespace dpl {

	class ParseTree {
	public:

		using node_type = std::variant<Token, std::pair<std::string_view, int>, std::monostate>;


		ParseTree(Grammar& g_) : grammar(g_) { }
		ParseTree(Grammar& g_, node_type n_) : grammar(g_), value(n_) { }


		bool operator<<(std::variant<Token, std::pair<std::string_view, int>> node) {
			if (!value.has_value()) {
				std::visit([&](const auto& v) { value.emplace(v); }, node);

				if (auto* const pair_val = std::get_if<std::pair<std::string_view, int>>(&value.value())) {
					const auto& rule = grammar[pair_val->first][pair_val->second];

					if (rule.isEpsilonProd()) {
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
			os << "\n\nParse Tree:\n===============\n";
			tree.recursivePrint("", true);
			os << "\n\n";

			return os;
		}


	private:

		void recursivePrint(const std::string& prefix, bool isLast) const {
			

			std::cout << prefix;
			std::cout << (isLast ? "'---" : "|---");

			if (value.has_value()) {
				if (const auto* nt = std::get_if<std::pair<std::string_view, int>>(&value.value())) std::cout << nt->first << "(" << nt->second << ")\n";
				else if (const auto* tkn = std::get_if<Token>(&value.value())) {
					dpl::log::coloredStream(std::cout, 0x03, (*tkn).stringify());
					std::cout << '\n';
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

		std::optional<node_type> value;
		std::vector<std::unique_ptr<ParseTree>> children;

		friend class BottomUpTreeBuilder;
		
	};

	
	class BottomUpTreeBuilder {
	public:

		BottomUpTreeBuilder(Grammar& g_) : grammar(g_) { };

		void addSubTree(ParseTree::node_type tree) {
			sub_trees.emplace_back(std::make_unique<ParseTree>(grammar, tree));
		}

		void packTree(ParseTree::node_type tree, size_t n) {
			std::unique_ptr<ParseTree> new_root{ std::make_unique<ParseTree>(grammar, tree) };
			for (int i = 0; i < n; i++) {
				new_root->children.emplace_back(std::move(sub_trees.back()));
				sub_trees.pop_back();
			}
			std::reverse(new_root->children.begin(), new_root->children.end());

			sub_trees.emplace_back(std::move(new_root));
		}

		void assignToTree(ParseTree& tree) {
			// #TASK : confirm there is exactly one sub-tree when accessing this
			if (sub_trees.empty()) return;
			tree.children = std::move(sub_trees.front()->children);
			tree.value = sub_trees.front()->value;
		}

	private:

		std::vector<std::unique_ptr<ParseTree>> sub_trees;
		Grammar& grammar;

	};

}