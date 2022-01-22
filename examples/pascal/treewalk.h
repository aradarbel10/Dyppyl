#pragma once

#include <dyppyl.h>

template<typename KeyT, typename ValT>
class ScopeStack {
public:
	using key_type = KeyT;
	using val_type = ValT;

private:
	std::deque<std::vector<std::pair<key_type, val_type>>> frames;

public:
	void enter_scope() { frames.push_back({}); }
	void exit_scope() { frames.pop_back(); }

	const auto& get_frames() const { return frames; }

	std::optional<val_type> lookup(key_type key) const {
		for (auto iter = frames.rbegin(); iter != frames.rend(); ++iter) {
			auto entry = std::find_if(iter->rbegin(), iter->rend(), [&](const auto& p) {
				return p.first == key;
			});

			if (entry != iter->rend()) return entry->second;
		}
		return std::nullopt;
	}

	void insert(key_type key, val_type val) {
		frames.back().push_back({ key, val });
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

class TreeWalker {
private:
	enum class TypeTag { None, Bool, Int, Func, Block };
	enum class Storage { Arg, Local, Const };

	friend auto& operator<<(std::ostream& os, TypeTag type) {
		if (type == TypeTag::Bool) os << "Bool";
		else if (type == TypeTag::Int) os << "Int";
		else if (type == TypeTag::Func) os << "Func";
		else if (type == TypeTag::Block) os << "Block";
		else os << "None";

		return os;
	}

	friend auto& operator<<(std::ostream& os, Storage storage) {
		if (storage == Storage::Arg) os << "Arg";
		else if (storage == Storage::Local) os << "Local";
		else if (storage == Storage::Const) os << "Const";

		return os;
	}

	using IRTree = dpl::Tree<dpl::Token<>>;

	struct Symbol {
		TypeTag typetag;
		const IRTree* def_addr;
	};

	struct FuncSymbol : public Symbol {
		std::vector<const IRTree*> args;
		int func_index = -1;

		FuncSymbol(const IRTree* addr_, std::vector<const IRTree*> args_)
			: Symbol(TypeTag::Func, addr_), args(args_) {}
	};

	struct BlockSymbol : public Symbol {
		std::vector<const IRTree*> locals;

		BlockSymbol(const IRTree* addr_, std::vector<const IRTree*> locals_)
			: Symbol(TypeTag::Block, addr_), locals(locals_) {}
	};

	struct VarSymbol : public Symbol {
		Storage storage;
		int offset;

		VarSymbol(const IRTree* addr_, Storage s_, int o_)
			: Symbol(TypeTag::Int, addr_), storage(s_), offset(o_) {}
	};

	// maps tree nodes to their semantic information
	std::map<const IRTree*, std::shared_ptr<Symbol>> symbol_table;
	

	void resolve_symbols(const IRTree& tree) {
		// maps scoped identifiers to their defining node
		ScopeStack<std::string_view, const IRTree*> scopes;
		resolve_symbols(tree, scopes);
	}

	TypeTag resolve_symbols(const IRTree& tree, ScopeStack<std::string_view, const IRTree*>& scopes) {
		using namespace dpl::literals;
		
		if (tree.value.name == "name"t) {
			auto result = scopes.lookup(std::get<std::string_view>(tree.value.value));
			if (!result) throw std::runtime_error{ "use of undefined identifier" };

			symbol_table[&tree] = symbol_table[*result];
			const auto symbol_entry = symbol_table[&tree];

			if (tree.children.empty()) { // no children means a variable
				if (symbol_entry->typetag != TypeTag::Int) throw std::runtime_error{ "missing parameter list" };
			} else { // yes children means a function node
				if (symbol_entry->typetag != TypeTag::Func) throw std::runtime_error{ "cant call a non-function" };

				const auto& params = tree[0];

				for (const auto& param : params.children) {
					TypeTag arg_type = resolve_symbols(param, scopes);
					if (arg_type != TypeTag::Int) throw std::runtime_error{ "cant pass non-integer parameters to function" };
				}

				if (params.children.size() != std::static_pointer_cast<FuncSymbol>(symbol_entry)->args.size())
					throw std::runtime_error{ "wrong amount of parameters" };
			}

			return TypeTag::Int;
		} else if (tree.value.name == "num"t) {
			return TypeTag::Int;
		} else if (tree.value.name == "add_op"t || tree.value.name == "mul_op"t || tree.value.name == "comp"t) {
			const auto& lhs = tree[0];
			const auto& rhs = tree[1];

			TypeTag lhs_type = resolve_symbols(lhs, scopes);
			TypeTag rhs_type = resolve_symbols(rhs, scopes);

			if (lhs_type != TypeTag::Int || rhs_type != TypeTag::Int)
				throw std::runtime_error{"cannot perform arithmeric on non-integer operands"};

			return (tree.value.name == "comp"t ? TypeTag::Bool : TypeTag::Int);
		} else if (tree.value == "block"tkn) {
			const auto& decls = tree[0];
			const auto& stmt = tree[1];

			std::vector<const IRTree*> locals_nodes;

			scopes.enter_scope();
			for (const auto& decl : decls.children)
				resolve_symbols(decl, scopes);
			
			const auto& curr_frame = scopes.get_frames().back();
			for (const auto& [name, node] : curr_frame) {
				if (symbol_table[node]->typetag == TypeTag::Int) {
					const auto as_var = std::static_pointer_cast<VarSymbol>(symbol_table[node]);
					if (as_var->storage == Storage::Local) {
						as_var->offset = locals_nodes.size();
						locals_nodes.push_back(as_var->def_addr);
					}
				}
			}

			symbol_table[&tree] = std::make_shared<BlockSymbol>(&tree, locals_nodes);
			
			resolve_symbols(stmt, scopes);
			scopes.exit_scope();

			return TypeTag::None;
		} else if (tree.value == "CONST"tkn) {
			for (const auto& const_decl : tree.children) {
				const auto& lhs = const_decl[0];

				scopes.insert(std::get<std::string_view>(lhs.value.value), &const_decl);

				symbol_table[&const_decl] = std::make_shared<VarSymbol>(&const_decl, Storage::Const, 0);
				symbol_table[&const_decl]->def_addr = &const_decl;
			}
		} else if (tree.value == "VAR"tkn) {
			for (const auto& var_decl : tree.children) {
				scopes.insert(std::get<std::string_view>(var_decl.value.value), &var_decl);
				symbol_table[&var_decl] = std::make_shared<VarSymbol>(&var_decl, Storage::Local, 0);
			}
		} else if (tree.value == "FUNCTION"tkn) {
			const auto& name = tree[0];
			const auto& args = tree[1];
			const auto& body = tree[2];

			scopes.insert(std::get<std::string_view>(name.value.value), &tree); // gotta lookup

			std::vector<const IRTree*> args_nodes;

			scopes.enter_scope();
			int arg_index = 0;
			for (const auto& arg : args.children) {
				scopes.insert(std::get<std::string_view>(arg.value.value), &arg);
				symbol_table[&arg] = std::make_shared<VarSymbol>(&arg, Storage::Arg, arg_index);
				args_nodes.push_back(&arg);

				++arg_index;
			}

			symbol_table[&tree] = std::make_shared<FuncSymbol>(&tree, std::move(args_nodes));

			resolve_symbols(body, scopes);
			scopes.exit_scope();
		} else if (tree.value == ":="tkn) {
			const auto& lhs = tree[0];
			const auto& rhs = tree[1];

			TypeTag lhs_type = resolve_symbols(lhs, scopes);
			TypeTag rhs_type = resolve_symbols(rhs, scopes);
			
			if (lhs_type != TypeTag::Int)
				throw std::runtime_error{ "cannot assign to a non-integer" };
			if (std::static_pointer_cast<VarSymbol>(symbol_table[&lhs])->storage != Storage::Local)
				throw std::runtime_error{ "cannot assign to a const / parameter value" };
			if (rhs_type != TypeTag::Int)
				throw std::runtime_error{ "cannot assign non-integer values" };
		} else if (tree.value == "BEGIN"tkn) {
			scopes.enter_scope();
			for (const auto& stmt : tree.children)
				resolve_symbols(stmt, scopes);

			scopes.exit_scope();
		} else if (tree.value == "IF"tkn || tree.value == "ELSE"tkn || tree.value == "WHILE"tkn) {
			const auto& cond = tree[0];
			const auto& body = tree[1];

			TypeTag cond_type = resolve_symbols(cond, scopes);
			if (cond_type != TypeTag::Bool)
				throw std::runtime_error{"branch condition must be a boolean"};
		
			scopes.enter_scope();
			resolve_symbols(body, scopes);
			scopes.exit_scope();

			if (tree.value == "ELSE"tkn) {
				const auto& alternative = tree[2];
				scopes.enter_scope();
				resolve_symbols(alternative, scopes);
				scopes.exit_scope();
			}
		} else if (tree.value == "RETURN"tkn) {
			const auto& frames = scopes.get_frames();
			const auto& expr = tree[0];

			bool found_enclosing = false;
			for (auto iter = frames.rbegin(); iter != frames.rend(); ++iter) {
				if (!iter->empty() && iter->back().second->value == "FUNCTION"tkn) {
					symbol_table[&tree] = symbol_table[iter->back().second];
					found_enclosing = true;
					break;
				}
			}

			if (!found_enclosing)
				throw std::runtime_error{ "return statement not inside a function" };

			TypeTag expr_type = resolve_symbols(expr, scopes);
			if (expr_type != TypeTag::Int)
				throw std::runtime_error{ "cant return non-integer value" };
		} else if (tree.value == "WRITE"tkn) {
			const auto& expr = tree[0];

			TypeTag expr_type = resolve_symbols(expr, scopes);
			if (expr_type != TypeTag::Int)
				throw std::runtime_error{ "cant write non-integer" };
		} else if (tree.value == "NOT"tkn) {
			const auto& expr = tree[0];
			TypeTag expr_type = resolve_symbols(expr, scopes);

			if (expr_type != TypeTag::Bool)
				throw std::runtime_error{ "cant negate a non-boolean value" };

			return TypeTag::Bool;
		} else if (tree.value == "ODD"tkn) {
			const auto& expr = tree[0];
			TypeTag expr_type = resolve_symbols(expr, scopes);

			if (expr_type != TypeTag::Int)
				throw std::runtime_error{"cant test parity of non-integer value"};

			return TypeTag::Bool;
		} else if (tree.value == "AND"tkn || tree.value == "OR"tkn) {
			const auto& lhs = tree[0];
			const auto& rhs = tree[1];

			TypeTag lhs_type = resolve_symbols(lhs, scopes);
			TypeTag rhs_type = resolve_symbols(rhs, scopes);

			if (lhs_type != TypeTag::Bool || rhs_type != TypeTag::Bool)
				throw std::runtime_error{ "cannot perform boolean operators on non-boolean operands" };

			return TypeTag::Bool;
		} else {
			throw std::runtime_error{"unhandled case"};
		}

		return TypeTag::None;
	}

	int label_id = 0;
	int func_id = 0;

	void codegen(const IRTree& tree)  {
		using namespace dpl::literals;

		if (tree.value.name == "name"t) {
			if (tree.children.empty()) { // no children means variable
				const auto& decl = std::static_pointer_cast<VarSymbol>(symbol_table[&tree]);
				if (decl->storage == Storage::Const) {
					std::cout << "\tmov\t$" << dpl::streamer{ (*decl->def_addr)[1].value.value } << ", \%eax"
							     " # from constant " << dpl::streamer{ (*decl->def_addr)[0].value.value } << '\n';
				} else if (decl->storage == Storage::Local) {
					std::cout << "\tmovl\t-" << 4 * (decl->offset + 1) << "(\%ebp), \%eax"
								 " # from local " << dpl::streamer{ decl->def_addr->value.value } << '\n';
				} else if (decl->storage == Storage::Arg) {
					throw std::runtime_error{ "not implemented" };
				}
			} else { // yes children means function call
				throw std::runtime_error{ "not implemented" };
			}
		} else if (tree.value.name == "num"t) {
			std::cout << "\tmov\t$" << dpl::streamer{ tree.value.value } << ", \%eax # number literal\n";
		} else if (tree.value.name == "add_op"t || tree.value.name == "mul_op"t || tree.value.name == "comp"t) {
			std::cout << "\n\t# binary expression " << std::get<std::string_view>(tree.value.value) << "\n";

			if (tree.value.name == "add_op"t) {
				codegen(tree[0]); // first operand
				std::cout << "\tpush\t\%eax\n";

				codegen(tree[1]); // second operand

				if (std::get<std::string_view>(tree.value.value) == "+"t) std::cout << "\taddl";
				else if (std::get<std::string_view>(tree.value.value) == "-"t) std::cout << "\tsubl";
				else throw std::runtime_error{ "unknown operator" };

				std::cout << "\t\%eax, (\%esp)\n";
				std::cout << "\tpop\t\%eax\n";
			} else if (tree.value.name == "mul_op"t) {
				codegen(tree[1]); // second operand
				std::cout << "\tpush\t\%eax\n";

				codegen(tree[0]); // first operand

				std::cout << "\n\tmovl\t$0, \%edx\n";
				if (std::get<std::string_view>(tree.value.value) == "*"t) std::cout << "\timull";
				else if (std::get<std::string_view>(tree.value.value) == "/"t) std::cout << "\tidivl";
				else throw std::runtime_error{ "unknown operator" };

				std::cout << "\t(\%esp)\n";
				std::cout << "\tadd\t$4, \%esp\n";
			} else if (tree.value.name == "comp"t) {
				codegen(tree[0]); // first operand
				std::cout << "\tpush\t\%eax\n";

				codegen(tree[1]); // second operand

				std::cout << "\n\tmovl\t$1, \%edx\n";
				std::cout << "\tcmp\t\%eax, (\%esp)\n\t";
				int label = label_id++;

				const auto& op = std::get<std::string_view>(tree.value.value);
				if (op == "<"t) std::cout << "jl";
				else if (op == ">"t) std::cout << "jg";
				else if (op == "="t) std::cout << "je";
				else if (op == ">="t) std::cout << "jge";
				else if (op == "<="t) std::cout << "jle";
				else if (op == "<>"t) std::cout << "jne";
				else throw std::runtime_error{ "unknown operator" };

				std::cout << "\t_Label" << label << " # evaluating boolean\n";
				std::cout << "\tmovl\t$0, \%edx\n\t_Label" << label << ":\n\tmovl\t\%edx, \%eax\n";
			}
			

			std::cout << "\t# end binary expression\n";
		} else if (tree.value == "block"tkn) {
			const auto& decls = tree[0];
			const auto& stmt = tree[1];

			const auto as_block = std::static_pointer_cast<BlockSymbol>(symbol_table[&tree]);
			
			std::cout << "\n\t# block\n";
			std::cout << "\tpush\t\%ebp\n\tmovl\t\%esp, \%ebp\n";
			std::cout << "\tsub\t$" << 4 * as_block->locals.size() << ", \%esp\n";
			codegen(stmt);
			std::cout << "\n\tmovl\t\%ebp, \%esp\n\tpop\t\%ebp\n";
			std::cout << "\t# end block\n";
		} else if (tree.value == ":="tkn) {
			std::cout << "\n\t# assignment\n";
			codegen(tree[1]);

			const auto as_var = std::static_pointer_cast<VarSymbol>(symbol_table[&tree[0]]);
			std::cout << "\tmovl\t\%eax, -" << 4 * (as_var->offset + 1) << "(\%ebp)\n";

			std::cout << "\t# end assignment\n";
		} else if (tree.value == "BEGIN"tkn) {
			for (const auto& stmt : tree.children)
				codegen(stmt);
		} else if (tree.value == "IF"tkn) {
			const auto& cond = tree[0];
			const auto& body = tree[1];

			codegen(cond);

			int label = label_id++;
			std::cout << "\tcmp\t$0, \%eax\n\tje\t_Label" << label << "\n\t# start of if-statement body\n";

			codegen(body);

			std::cout << "\t_Label" << label << ":\n\t# end of if-statement\n";
		} else if (tree.value == "WHILE"tkn) {
			const auto& cond = tree[0];
			const auto& body = tree[1];

			int cond_label = label_id++;
			int exit_label = label_id++;

			std::cout << "\t_Label" << cond_label << ":\n";
			codegen(cond);
			std::cout << "\tcmp\t$0, \%eax\n\tje\t_Label" << exit_label << "\n\t# start of while body\n";

			codegen(body);

			std::cout << "\tjmp\t_Label" << cond_label << "\n\t_Label" << exit_label << ":\n\t# end of while loop\n";
		} /*else if (tree.value.tkn == "RETURN"tkn) {
			codegen(tree[0]);
			std::cout << "code: ret\n";
		}*/ else if (tree.value == "WRITE"tkn) {
			std::cout << "\n\t# write\n";
			codegen(tree[0]);

			std::cout << "\tpush\t\%eax\n";
			std::cout << "\tcall\t_print_ui32\n";

			std::cout << "\tpush\t$\'\\n\n";
			std::cout << "\tcall\t_print_char\n";
			std::cout << "\tadd\t$8, \%esp\n";
			std::cout << "\t# end write\n";
		}/* else if (tree.value.tkn == "NOT"tkn) {
			codegen(tree[0]);
			int label_neg = label_id++;
			int label_end = label_id++;

			std::cout << "code: jz _Label" << label_neg << "\n";

			std::cout << "code: pop\n";
			std::cout << "code: push 0\n";

			std::cout << "code: jmp _Label" << label_end << "\n";
			std::cout << "code: _Label" << label_neg << ":\n";

			std::cout << "code: pop\n";
			std::cout << "code: push 1\n";

			std::cout << "code: _Label" << label_end << ":\n";
		} else if (tree.value.tkn == "ODD"tkn) {
			std::cout << "code: push 1\n";
			std::cout << "code: bitand\n";
		} else if (tree.value.tkn == "AND"tkn || tree.value.tkn == "OR"tkn) {
			
		}*/ else {
			throw std::runtime_error{"unhandled case"};
		}
	}

	void toplevel_codegen(const IRTree& tree) {
		std::cout << ".globl\t_start\n\n\n.text\n_start:\n\t# entry point\n\nmovl\t\%esp, \%ebp\n\n";
		codegen(tree);
		std::cout << "\t# end of program\n\tmovl\t$0, \%ebx\n\tmovl\t$1, \%eax\n\tint\t$0x80\n";
	}


	void print_symbol_table(std::ostream& os) const {
		using namespace dpl::literals;

		os << std::left << std::setfill(' ')
			<< std::setw(16) << "address"
			<< std::setw(24) << "node"
			<< std::setw(16) << "def"
			<< std::setw(10) << "type\n\n";
		for (auto&& [node, symbol] : symbol_table) {
			os << std::left << std::setfill(' ')
				<< std::setw(16) << (reinterpret_cast<std::uintptr_t>(node) & 0xFFFF)
				<< std::setw(24) << dpl::streamer{
					(node->value == "="tkn || node->value == "FUNCTION"tkn ? (*node)[0].value.value : node->value.value) }
				<< std::setw(16) << (reinterpret_cast<std::uintptr_t>(symbol->def_addr) & 0xFFFF)
				<< std::setw(10) << symbol->typetag;

			if (symbol->typetag == TypeTag::Int) {
				const auto as_var = std::static_pointer_cast<VarSymbol>(symbol);
				os << std::setw(16) << as_var->storage
				   << std::setw(10) << as_var->offset;
			} else if (symbol->typetag == TypeTag::Func) {
				const auto as_func = std::static_pointer_cast<FuncSymbol>(symbol);

				os << "(";
				for (const auto* arg : as_func->args) {
					os << dpl::streamer{ arg->value.value } << ",";
				}
				os << ")";
			}

			os << "\n";
		}
	}

public:

	auto interpret_tree(const dpl::ParseTree<>& tree_) {
		// convert parse tree to an IR tree
		IRTree tree = dpl::tree_map(tree_, [](auto&& val) {
			if (auto* tkn = std::get_if<dpl::Token<>>(&val)) {
				return *tkn;
			} else if (auto* rule = std::get_if<dpl::RuleRef<>>(&val)) {
				dpl::Token<> tkn;
				tkn.name = rule->get_name();
				tkn.value = rule->get_name();
				tkn.type = dpl::Token<>::Type::value;
				return tkn;
			}
		});

		resolve_symbols(tree);
		std::cout << "\n\n\nSymbol Table:\n";
		print_symbol_table(std::cout);

		std::cout << "\n\n\nGenerated Code:\n";
		codegen(tree);
	}
};