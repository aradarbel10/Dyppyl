#pragma once



class TreeWalker {
public:
	enum class TypeTag { None, Bool, Int, Func };

	struct Node {
		Node() = default;
		Node(const dpl::Token<>& tkn_) : tkn(tkn_) {}

		dpl::Token<> tkn{};
		TypeTag typetag = TypeTag::None;
		const dpl::Tree<Node>* symbol_addr = nullptr;

		auto get_str() { return std::get<std::string_view>(tkn.value); }
		auto get_num() { return std::get<long double>(tkn.value); }

		friend auto& operator<<(std::ostream& os, Node node) {
			os << node.tkn;

			if (node.typetag != TypeTag::None) os << ":";

			if (node.typetag == TypeTag::Bool) os << "Bool";
			else if (node.typetag == TypeTag::Int) os << "Int";
			else if (node.typetag == TypeTag::Func) os << "Func";

			if (node.symbol_addr != nullptr) os << " at {" << (reinterpret_cast<std::uintptr_t>(node.symbol_addr) & 0xFFFF) << "}";

			return os;
		}
	};

	using IRTree = dpl::Tree<Node>;

	class SymbolTable {
	public:
		using key_type = std::string_view;
		using val_type = IRTree*;

	private:
		std::deque<std::vector<std::pair<key_type, val_type>>> scopes;

	public:
		void enter_scope() { scopes.push_back({}); }
		void exit_scope() { scopes.pop_back(); }

		const auto& get_scopes() const { return scopes; }

		std::optional<val_type> lookup(key_type key) const {
			for (auto iter = scopes.rbegin(); iter != scopes.rend(); ++iter) {
				auto entry = std::find_if(iter->rbegin(), iter->rend(), [&](const auto& p) {
					return p.first == key;
				});

				if (entry != iter->rend()) return entry->second;
			}
			return std::nullopt;
		}

		void insert(key_type key, val_type val) {
			scopes.back().push_back({ key, val });
		}

		std::optional<val_type> lookup_or_insert(key_type key, val_type val) {
			auto result = lookup(key);

			if (result) return result;
			else {
				insert(key, val);
				return std::nullopt;
			}
		}
	};

private:

	void from_parse_tree(IRTree& dest, const dpl::ParseTree<>& src) {
		if (auto* tkn_ = std::get_if<dpl::Token<>>(&src.value)) {
			dest.value.tkn = *tkn_;
		} else if (auto* rule = std::get_if<dpl::RuleRef<>>(&src.value)) {
			dest.value.tkn.name = rule->get_name();
			dest.value.tkn.value = rule->get_name();
			dest.value.tkn.type = dpl::Token<>::Type::value;
		}

		for (const auto& sub_ : src.children) {
			IRTree sub;
			from_parse_tree(sub, sub_);
			dest.children.push_back(sub);
		}
	}

	

	SymbolTable symbol_table;

	void resolve_symbols(IRTree& tree) {
		using namespace dpl::literals;

		if (tree.value.tkn.name == "name"t) {
			auto result = symbol_table.lookup(tree.value.get_str());
			if (!result) throw std::runtime_error{ "use of undefined identifier" };

			tree.value.symbol_addr = *result;
			tree.value.typetag = TypeTag::Int;

			if (tree.children.empty()) {
				if ((*result)->value.typetag == TypeTag::Func) throw std::runtime_error{ "missing parameter list" };
			} else {
				auto& params = tree.children[0];

				for (auto& param : params.children) {
					resolve_symbols(param);
					if (param.value.typetag != TypeTag::Int) throw std::runtime_error{ "cant pass non-integer parameters to function" };
				}

				if (tree[0].children.size() != (**result)[1].children.size())
					throw std::runtime_error{ "wrong amount of parameters" };
			}
		} else if (tree.value.tkn.name == "num"t) {
			tree.value.typetag = TypeTag::Int;
		} else if (tree.value.tkn.name == "add_op"t || tree.value.tkn.name == "mul_op"t || tree.value.tkn.name == "comp"t) {
			auto& lhs = tree.children[0];
			auto& rhs = tree.children[1];

			resolve_symbols(lhs);
			resolve_symbols(rhs);

			if (lhs.value.typetag != TypeTag::Int || rhs.value.typetag != TypeTag::Int)
				throw std::runtime_error{"cannot perform arithmeric on non-integer operands"};

			tree.value.typetag = (tree.value.tkn.name == "comp"t ? TypeTag::Bool : TypeTag::Int);
		} else if (tree.value.tkn == "block"tkn) {
			auto& decls = tree[0];
			auto& stmt = tree[1];

			symbol_table.enter_scope();
			for (auto& decl : decls.children)
				resolve_symbols(decl);
			
			resolve_symbols(stmt);
			symbol_table.exit_scope();
		} else if (tree.value.tkn == "CONST"tkn) {
			for (auto& const_decl : tree.children) {
				auto& lhs = const_decl[0];

				symbol_table.insert(lhs.value.get_str(), &const_decl);
				lhs.value.typetag = TypeTag::Int;
				const_decl.value.symbol_addr = &const_decl;
			}
		} else if (tree.value.tkn == "VAR"tkn) {
			for (auto& var_decl : tree.children) {
				symbol_table.insert(var_decl.value.get_str(), &var_decl);
				var_decl.value.typetag = TypeTag::Int;
				var_decl.value.symbol_addr = &var_decl;
			}
		} else if (tree.value.tkn == "FUNCTION"tkn) {
			auto& name = tree.children[0];
			auto& args = tree.children[1];
			auto& body = tree.children[2];

			symbol_table.insert(name.value.get_str(), &tree);
			name.value.typetag = TypeTag::Func;
			tree.value.symbol_addr = &tree;

			symbol_table.enter_scope();
			for (auto& arg : args.children) {
				symbol_table.insert(arg.value.get_str(), &arg);
				arg.value.typetag = TypeTag::Int;
			}

			resolve_symbols(body);
			symbol_table.exit_scope();
		} else if (tree.value.tkn == ":="tkn) {
			auto& lhs = tree.children[0];
			auto& rhs = tree.children[1];

			resolve_symbols(lhs);
			resolve_symbols(rhs);
			if (rhs.value.typetag != TypeTag::Int) throw std::runtime_error{"cannot assign non-integer values"};
		} else if (tree.value.tkn == "BEGIN"tkn) {
			symbol_table.enter_scope();
			for (auto& stmt : tree.children)
				resolve_symbols(stmt);

			symbol_table.exit_scope();
		} else if (tree.value.tkn == "IF"tkn || tree.value.tkn == "WHILE"tkn) {
			auto& cond = tree.children[0];
			auto& body = tree.children[1];

			resolve_symbols(cond);
			if (cond.value.typetag != TypeTag::Bool) throw std::runtime_error{"branch condition must be a boolean"};
		
			symbol_table.enter_scope();
			resolve_symbols(body);
			symbol_table.exit_scope();
		} else if (tree.value.tkn == "RETURN"tkn) {
			const auto& scopes = symbol_table.get_scopes();
			auto& expr = tree.children[0];

			for (auto iter = scopes.rbegin(); iter != scopes.rend(); ++iter) {
				if (!iter->empty() && iter->back().second->value.tkn == "FUNCTION"tkn) {
					tree.value.symbol_addr = iter->back().second;
					break;
				}
			}

			resolve_symbols(expr);
			if (expr.value.typetag != TypeTag::Int) throw std::runtime_error{"cant return non-integer value"};
		} else if (tree.value.tkn == "WRITE"tkn) {
			auto& expr = tree.children[0];
			resolve_symbols(expr);
		} else if (tree.value.tkn == "NOT"tkn) {
			auto& expr = tree.children[0];
			resolve_symbols(expr);
			if (expr.value.typetag != TypeTag::Bool) throw std::runtime_error{"cant negate non-boolean value"};
			tree.value.typetag = TypeTag::Bool;
		} else if (tree.value.tkn == "ODD"tkn) {
			auto& expr = tree.children[0];
			resolve_symbols(expr);
			if (expr.value.typetag != TypeTag::Int) throw std::runtime_error{"cant test parity of non-integer value"};
			tree.value.typetag = TypeTag::Bool;
		} else if (tree.value.tkn == "AND"tkn || tree.value.tkn == "OR"tkn) {
			auto& lhs = tree.children[0];
			auto& rhs = tree.children[1];

			resolve_symbols(lhs);
			resolve_symbols(rhs);

			if (lhs.value.typetag != TypeTag::Bool || rhs.value.typetag != TypeTag::Bool)
				throw std::runtime_error{ "cannot perform boolean operators on non-boolean operands" };

			tree.value.typetag = TypeTag::Bool;
		} else {
			throw std::runtime_error{"unhandled case"};
		}
	}

public:

	auto interpret_tree(const dpl::ParseTree<>& tree_) {
		// convert parse tree to an IR tree
		IRTree tree;
		from_parse_tree(tree, tree_);

		resolve_symbols(tree);

		std::cout << "\n\n\nSemantic Tree:\n" << tree;
	}
};