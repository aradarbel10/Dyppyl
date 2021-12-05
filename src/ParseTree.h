#pragma once

#include "tokenizer/Token.h"
#include "Grammar.h"

#include <iostream>

#include <array>
#include <variant>
#include <optional>
#include <span>
#include <type_traits>

namespace dpl {

	template <typename T>
	class Tree {
	private:
		
		void recursivePrint(const std::string& prefix, bool isLast) const {
			std::cout << prefix;
			std::cout << (isLast ? "'---" : "|---");

			std::cout << value;

			for (int i = 0; i < children.size(); i++) {
				children[i].recursivePrint(prefix + (isLast ? "    " : "|   "), i == children.size() - 1);
			}
		}

	protected:
		std::optional<T> value;
		std::vector<Tree<T>> children;

	public:
		using node_type = T;

		Tree() = default;
		Tree(T node) : value(node) { }
		Tree(T node, std::initializer_list<Tree<T>> cs) : value(node), children(cs) { }

		friend std::ostream& operator<<(std::ostream& os, const Tree<T>& tree) {
			os << "\n\nParse Tree:\n===============\n";
			tree.recursivePrint("", true);
			os << "\n\n";

			return os;
		}

		friend bool operator==(const Tree<T>& lhs, const Tree<T>& rhs) {
			return (lhs.value == rhs.value) && (lhs.children == rhs.children);
		}


	};

	typedef dpl::Tree<std::variant<std::string_view, dpl::Terminal>> TreePattern;

	class ParseTree : public dpl::Tree<std::variant<std::string_view, Token, RuleRef, std::monostate>> {
	private:
		using parent = dpl::Tree<std::variant<std::string_view, Token, RuleRef, std::monostate>>;
		using parent::parent;

		friend class BottomUpTreeBuilder;
		friend class TopDownTreeBuilder;
	};

	inline std::ostream& operator<<(std::ostream& os, const std::optional<std::variant<std::string_view, Token, RuleRef, std::monostate>> value) {
		if (value.has_value()) {
			if (const auto* nt = std::get_if<RuleRef>(&value.value())) std::cout << nt->name << "(" << nt->prod << ")\n";
			else if (const auto* tkn = std::get_if<Token>(&value.value())) {
				dpl::log::coloredStream(std::cout, 0x03, (*tkn).stringify());
				std::cout << '\n';
			} else {
				std::cout << "null\n";
			}
		}
		return os;
	}
	
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
			sub_trees.emplace_back(tree);
		}

		void packTree(ParseTree::node_type tree, size_t n) {
			ParseTree new_root = ParseTree{ tree };

			for (int i = 0; i < n; i++) {
				new_root.children.emplace_back(std::move(sub_trees.back()));
				sub_trees.pop_back();
			}
			std::reverse(new_root.children.begin(), new_root.children.end());

			sub_trees.emplace_back(std::move(new_root));
		}

		void pushNode(ParseTree::node_type node) override {
			if (const auto* prod = std::get_if<RuleRef>(&node)) {
				packTree(node, prod->getRule().size());
			} else if (std::holds_alternative<Token>(node)) {
				addSubTree(node);
			}
		}

		void assignToTree(ParseTree& tree) override {
			if (sub_trees.size() != 1) return;
			tree.children = std::move(sub_trees.front().children);
			tree.value = sub_trees.front().value;
		}

	private:

		std::vector<ParseTree> sub_trees;

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

				if (auto* const prod = std::get_if<RuleRef>(&tree.value.value())) {
					const auto& rule = prod->getRule();

					if (rule.isEpsilonProd()) {
						tree.children.reserve(1);
						tree.children.push_back({ std::monostate{} });
					} else {
						tree.children.reserve(rule.size());
						for (int i = 0; i < tree.children.capacity(); i++) {
							tree.children.push_back(ParseTree{});
						}
					}
				}

				return true;
			}

			for (auto& child : tree.children) {
				if (pushNode(node, static_cast<ParseTree&>(child))) return true;
			}

			return false;
		}

		void assignToTree(ParseTree& tree) override {
			tree = std::move(inner_tree);
			inner_tree.value.reset();
		}

	private:

		ParseTree inner_tree;

	};

}