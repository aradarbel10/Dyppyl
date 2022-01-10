#pragma once

#include "tokenizer/Token.h"
#include "Grammar.h"

#include <iostream>

#include <array>
#include <variant>
#include <optional>
#include <span>
#include <type_traits>
#include <stack>
#include <ranges>

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

		Tree() = default;
		Tree(T node) : value(node) {}
		Tree(T node, std::initializer_list<Tree<T>> cs) : value(node), children(cs) {}

		auto& operator[](size_t index) { return children[index]; }
		const auto& operator[](size_t index) const { return children[index]; }

		friend std::ostream& operator<<(std::ostream& os, const Tree<T>& tree) {
			tree.recursivePrint(os, "", true);
			return os;
		}

		friend bool operator==(const Tree<T>& lhs, const Tree<T>& rhs) {
			return (lhs.value == rhs.value) && (lhs.children == rhs.children);
		}

		void apply_modifiers(const std::vector<TreeModifier>& modifiers) {
			int child_index = 0;

			for (auto modifier : modifiers) {
				if (modifier == TreeModifier::None) {
					child_index++;
				} else if (modifier == TreeModifier::Hide) {
					children.erase(children.begin() + child_index);
				} else {
					if (modifier == TreeModifier::Lift) {
						value = children[child_index].value;
					} // else it's TreeModifier::Adopt, just keep going

					int grandchildren_count = children[child_index].children.size();
					children.insert(children.begin() + child_index, children[child_index].children.begin(), children[child_index].children.end());
					child_index += grandchildren_count;
					children.erase(children.begin() + child_index);
				}
			}
		}

		std::optional<std::vector<Tree<T>>> match(const Tree<T>& pattern) const {
			if (pattern.is_wildcard) return std::vector{ *this };

			if constexpr (requires { pattern.value != value; }) {
				if (pattern.value != value)
					return std::nullopt;
			} else return std::nullopt;
				

			if (pattern.children.size() != children.size()) {
				if (pattern.children.size() != 0) return std::nullopt;
				else return std::vector{ *this };
			}

			std::vector<Tree<T>> result;
			for (int i = 0; i < children.size(); i++) {
				auto subchildren = children[i].match(pattern.children[i]);

				if (subchildren) std::for_each(subchildren->begin(), subchildren->end(), [&](Tree<T>& child) { result.push_back(std::move(child)); });
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
		&& std::convertible_to<std::invoke_result_t<Func, Tree<T>>, Tree<T>>
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


	template<typename GrammarT = dpl::Grammar<>>
	using ParseTree = dpl::Tree<std::variant<
		std::monostate,
		typename GrammarT::nonterminal_type,
		typename GrammarT::token_type,
		RuleRef<GrammarT>
	>>;

	template<typename GrammarT = dpl::Grammar<>>
	inline std::ostream& operator<<(std::ostream& os, const std::optional<typename ParseTree<GrammarT>::node_type> value) {
		if (value.has_value()) {
			if (const auto* nt = std::get_if<RuleRef<GrammarT>>(&value.value())) os << nt->get_name() << "(" << nt->get_prod() << ")";
			else if (const auto* tkn = std::get_if<typename GrammarT::token_type>(&value.value())) {
				dpl::colored_stream(os, 0x03, (*tkn).stringify());
			} else {
				os << "null";
			}
		}
		return os;
	}
	
	template<typename GrammarT = dpl::Grammar<>>
	class TreeBuilder {
	public:
		using grammar_type = GrammarT;
		using tree_type = ParseTree<GrammarT>;

	protected:
		grammar_type& grammar;

	public:

		TreeBuilder(grammar_type& g_) : grammar(g_) { };
		virtual void pushNode(typename tree_type::node_type) = 0;
		virtual void assignToTree(tree_type&) = 0;

	};

	template<typename GrammarT = dpl::Grammar<>>
	class BottomUpTreeBuilder : public TreeBuilder<GrammarT> {
	public:
		using grammar_type = GrammarT;
		using tree_type = dpl::ParseTree<grammar_type>;

	private:
		std::vector<tree_type> sub_trees;

	public:

		BottomUpTreeBuilder(grammar_type& g_) : TreeBuilder<grammar_type>(g_) { };

		void addSubTree(typename tree_type::node_type tree) {
			sub_trees.emplace_back(tree);
		}

		void packTree(tree_type::node_type node, size_t n) {
			auto new_root = tree_type{ node };

			for (int i = 0; i < n; i++) {
				new_root.children.emplace_back(std::move(sub_trees.back()));
				sub_trees.pop_back();
			}
			std::reverse(new_root.children.begin(), new_root.children.end());

			sub_trees.emplace_back(std::move(new_root));

			if (auto* rule = std::get_if<RuleRef<grammar_type>>(&node)) {
				sub_trees.back().apply_modifiers(rule->getRule().tree_modifiers);
			}
		}

		void pushNode(typename tree_type::node_type node) override {
			if (const auto* prod = std::get_if<RuleRef<grammar_type>>(&node)) {
				packTree(node, prod->getRule().size());
			} else if (std::holds_alternative<typename grammar_type::token_type>(node)) {
				addSubTree(node);
			}
		}

		void assignToTree(tree_type& tree) override {
			if (sub_trees.size() == 1) {
				tree.children = std::move(sub_trees.front().children);
				tree.value = sub_trees.front().value;
			}

			sub_trees.clear();
		}
	};

	template<typename GrammarT = dpl::Grammar<>>
	class TopDownTreeBuilder : public TreeBuilder<GrammarT> {
	public:
		using grammar_type = GrammarT;
		using tree_type = dpl::ParseTree<grammar_type>;

	private:
		tree_type inner_tree;
		std::stack<tree_type*> last_inserted;

	public:

		TopDownTreeBuilder(grammar_type& g) : TreeBuilder<grammar_type>(g) {}

		void pushNode(typename tree_type::node_type node) override {
			if (last_inserted.empty()) {
				inner_tree.value = node;
				last_inserted.push(&inner_tree);
				return;
			}
			
			if (auto* rule = std::get_if<RuleRef<grammar_type>>(&last_inserted.top()->value)) {
				if (last_inserted.top()->children.size() < rule->getRule().size()) {
					last_inserted.top()->children.push_back(node);
					last_inserted.push(&last_inserted.top()->children.back());
					return;
				} else {
					last_inserted.top()->apply_modifiers(rule->getRule().tree_modifiers);
				}
			}

			last_inserted.pop();
			if (!last_inserted.empty()) pushNode(node);
		}

		void assignToTree(tree_type& tree) override {
			tree = std::move(inner_tree);
			inner_tree = tree_type{};
			
			std::stack<tree_type*>{}.swap(last_inserted);
		}
	};

}