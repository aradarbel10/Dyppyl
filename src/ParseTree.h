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


		ParseTree() {};
		ParseTree(node_type n_) : value(n_) { }

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

		std::optional<node_type> value;
		std::vector<std::unique_ptr<ParseTree>> children;

		friend class BottomUpTreeBuilder;
		friend class TopDownTreeBuilder;
		
	};

	
	class TreeBuilder {
	public:

		TreeBuilder(Grammar& g_) : grammar(g_) { };
		virtual void pushNode(ParseTree::node_type) = 0;
		virtual void assignToTree(ParseTree&) = 0;

	protected:

		Grammar& grammar;

	};

	class BottomUpTreeBuilder : public TreeBuilder {
	public:

		BottomUpTreeBuilder(Grammar& g_) : TreeBuilder(g_) { };

		void addSubTree(ParseTree::node_type tree) {
			sub_trees.emplace_back(std::make_unique<ParseTree>(tree));
		}

		void packTree(ParseTree::node_type tree, size_t n) {
			std::unique_ptr<ParseTree> new_root{ std::make_unique<ParseTree>(tree) };
			for (int i = 0; i < n; i++) {
				new_root->children.emplace_back(std::move(sub_trees.back()));
				sub_trees.pop_back();
			}
			std::reverse(new_root->children.begin(), new_root->children.end());

			sub_trees.emplace_back(std::move(new_root));
		}

		void pushNode(ParseTree::node_type node) override {
			if (const auto* prod = std::get_if<std::pair<std::string_view, int>>(&node)) {
				packTree(node, grammar[prod->first][prod->second].size());
			} else if (std::holds_alternative<Token>(node)) {
				addSubTree(node);
			}
		}

		void assignToTree(ParseTree& tree) override {
			if (sub_trees.size() != 1) return;
			tree.children = std::move(sub_trees.front()->children);
			tree.value = sub_trees.front()->value;
		}

	private:

		std::vector<std::unique_ptr<ParseTree>> sub_trees;

	};


	class TopDownTreeBuilder : public TreeBuilder {
	public:

		TopDownTreeBuilder(Grammar& g) : TreeBuilder(g) { }

		void pushNode(ParseTree::node_type node) override {
			pushNode(node, inner_tree);
		}

		bool pushNode(ParseTree::node_type node, ParseTree& tree) {
			if (!tree.value.has_value()) {
				std::visit([&](const auto& v) { tree.value.emplace(v); }, node);

				if (auto* const pair_val = std::get_if<std::pair<std::string_view, int>>(&tree.value.value())) {
					const auto& rule = grammar[pair_val->first][pair_val->second];

					if (rule.isEpsilonProd()) {
						tree.children.reserve(1);
						tree.children.push_back(std::move(std::make_unique<ParseTree>(std::monostate{})));
					} else {
						tree.children.reserve(rule.size());
						for (int i = 0; i < tree.children.capacity(); i++) {
							tree.children.push_back(std::move(std::make_unique<ParseTree>()));
						}
					}
				}

				return true;
			}

			for (auto& child : tree.children) {
				if (pushNode(node, *child)) return true;
			}

			return false;
		}

		void assignToTree(ParseTree& tree) override {
			tree = std::move(inner_tree);
		}

	private:

		ParseTree inner_tree;

	};

}