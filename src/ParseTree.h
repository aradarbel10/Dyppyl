#pragma once

#include "tokenizer/Token.h"
#include "Grammar.h"

#include <iostream>

#include <array>
#include <variant>
#include <optional>
#include <span>
#include <type_traits>

// #TASK : improve this macro into a mini-edsl
#define call_by_traversal_order(root, children, ord)	\
	switch (ord) {										\
	case TraverseOrder::BottomUp:						\
		children();										\
		root();											\
		break;											\
	case TraverseOrder::TopDown:						\
		root();											\
		children();										\
		break;											\
	}													\

namespace dpl {

	enum class TraverseOrder { TopDown, BottomUp };

	template <typename T>
	class Tree {
	private:
		
		template <typename S> friend class Tree;

		void recursivePrint(std::ostream& os, const std::string& prefix, bool isLast) const {
			os << prefix;
			os << (isLast ? "'---" : "|---");

			os << dpl::streamer{ value } << "\n";

			for (int i = 0; i < children.size(); i++) {
				children[i].recursivePrint(os, prefix + (isLast ? "    " : "|   "), i == children.size() - 1);
			}
		}

		bool is_wildcard = false;

	public:
		T value;
		std::vector<Tree<T>> children;
	
		using node_type = T;

		Tree() : is_wildcard(true) { }
		Tree(T node) : value(node) { }
		Tree(T node, std::initializer_list<Tree<T>> cs) : value(node), children(cs) { }

		auto& operator[](size_t index) { return children[index]; }
		const auto& operator[](size_t index) const { return children[index]; }

		template <typename T> requires requires (T t) { std::get<T>(value); }
		T& get() {
			return std::get<T>(value);
		}

		friend std::ostream& operator<<(std::ostream& os, const Tree<T>& tree) {
			tree.recursivePrint(os, "", true);
			return os;
		}

		friend bool operator==(const Tree<T>& lhs, const Tree<T>& rhs) {
			return (lhs.value == rhs.value) && (lhs.children == rhs.children);
		}

		std::optional<std::vector<Tree<T>>> match(const Tree<T>& pattern) const {
			if (pattern.is_wildcard) return std::vector{ *this };

			if (!std::visit([]<typename T1, typename T2> (const T1 & lhs, const T2 & rhs) {
				if constexpr (!std::equality_comparable_with<T1, T2>) return false;
				else return lhs == rhs;
			}, pattern.value, value)) return std::nullopt;

			if (pattern.children.size() != children.size()) {
				if (pattern.children.size() != 0) return std::nullopt;
				else return std::vector{ *this };
			}

			std::vector<Tree<T>> result;
			for (int i = 0; i < children.size(); i++) {
				auto subchildren = children[i].match(pattern.children[i]);

				if (subchildren) std::ranges::for_each(*subchildren, [&](Tree<T>& child) { result.push_back(std::move(child)); });
				else return std::nullopt;
			}

			return result;
		}

		void replace(const Tree<T>& pattern, const T& node, TraverseOrder order = TraverseOrder::BottomUp) {
			const auto apply_root = [&] {
				if (auto subs = this->match(pattern)) {
					value = node;
					children = *subs;
				}
			};

			const auto apply_children = [&] {
				for (auto& child : children) {
					child.replace(pattern, node);
				}
			};

			call_by_traversal_order(apply_root, apply_children, order);
		}

		template <typename Func> requires std::invocable<Func, Tree<T>>
		&& std::is_same_v<std::invoke_result_t<Func, Tree<T>>, Tree<T>>
		void replace_with(const Tree<T>& pattern, const Func& transformer, TraverseOrder order = TraverseOrder::BottomUp) {
			const auto apply_root = [&] {
				if (this->match(pattern)) {
					*this = transformer(*this);
				}
			};

			const auto apply_children = [&] {
				for (auto& child : children) {
					child.replace_with(pattern, transformer, order);
				}
			};
			
			call_by_traversal_order(apply_root, apply_children, order);
		}
	};

	template <typename T, typename Func> requires std::invocable<Func, Tree<T>&>
	void tree_visit(Tree<T>& tree, const Func& func, TraverseOrder order = TraverseOrder::BottomUp) {
		const auto apply_root = [&] {
			func(tree);
		};

		const auto apply_children = [&] {
			for (auto& child : tree.children) {
				tree_visit(child, func);
			}
		};
		
		call_by_traversal_order(apply_root, apply_children, order);
	}

	template<typename AtomT = char>
	using ParseTree = dpl::Tree<std::variant<std::monostate, std::string_view, Token<>, RuleRef<AtomT>>>;

	template<typename AtomT>
	inline std::ostream& operator<<(std::ostream& os, const std::optional<typename ParseTree<AtomT>::node_type> value) {
		if (value.has_value()) {
			if (const auto* nt = std::get_if<RuleRef<AtomT>>(&value.value())) os << nt->name << "(" << nt->prod << ")";
			else if (const auto* tkn = std::get_if<Token>(&value.value())) {
				dpl::colored_stream(os, 0x03, (*tkn).stringify());
			} else {
				os << "null";
			}
		}
		return os;
	}
	
	template<typename AtomT = char>
	class TreeBuilder {
	public:

		TreeBuilder(Grammar<AtomT>& g_) : grammar(g_) { };
		virtual void pushNode(ParseTree<AtomT>::node_type) = 0;
		virtual void assignToTree(ParseTree<AtomT>&) = 0;

	protected:

		Grammar<AtomT>& grammar;

	};

	template<typename AtomT = char>
	class BottomUpTreeBuilder : public TreeBuilder<AtomT> {
	public:

		BottomUpTreeBuilder(Grammar<AtomT>& g_) : TreeBuilder(g_) { };

		void addSubTree(ParseTree<AtomT>::node_type tree) {
			sub_trees.emplace_back(tree);
		}

		void packTree(ParseTree<AtomT>::node_type tree, size_t n) {
			auto new_root = ParseTree<AtomT>{ tree };

			for (int i = 0; i < n; i++) {
				new_root.children.emplace_back(std::move(sub_trees.back()));
				sub_trees.pop_back();
			}
			std::reverse(new_root.children.begin(), new_root.children.end());

			sub_trees.emplace_back(std::move(new_root));
		}

		void pushNode(ParseTree<AtomT>::node_type node) override {
			if (const auto* prod = std::get_if<RuleRef<AtomT>>(&node)) {
				packTree(node, prod->getRule().size());
			} else if (std::holds_alternative<Token>(node)) {
				addSubTree(node);
			}
		}

		void assignToTree(ParseTree<AtomT>& tree) override {
			if (sub_trees.size() == 1) {
				tree.children = std::move(sub_trees.front().children);
				tree.value = sub_trees.front().value;
			}

			sub_trees.clear();
		}

	private:

		std::vector<ParseTree<AtomT>> sub_trees;

	};

	template<typename AtomT = char>
	class TopDownTreeBuilder : public TreeBuilder<AtomT> {
	public:

		TopDownTreeBuilder(Grammar<AtomT>& g) : TreeBuilder(g) { }

		void pushNode(ParseTree<AtomT>::node_type node) override {
			pushNode(node, inner_tree);
		}

		bool pushNode(ParseTree<AtomT>::node_type node, ParseTree<AtomT>& tree) {
			if (std::holds_alternative<std::monostate>(tree.value)) {
				tree.value = node;

				if (auto* const prod = std::get_if<RuleRef<AtomT>>(&tree.value)) {
					const auto& rule = prod->getRule();

					if (rule.isEpsilonProd()) {
						tree.children.reserve(1);
						tree.children.push_back({ std::monostate{} });
					} else {
						tree.children.reserve(rule.size());
						for (int i = 0; i < tree.children.capacity(); i++) {
							tree.children.push_back(ParseTree<AtomT>{});
						}
					}
				}

				return true;
			}

			for (auto& child : tree.children) {
				if (pushNode(node, child)) return true;
			}

			return false;
		}

		void assignToTree(ParseTree<AtomT>& tree) override {
			tree = std::move(inner_tree);
			inner_tree = ParseTree<AtomT>{};
		}

	private:

		ParseTree<AtomT> inner_tree;

	};

}