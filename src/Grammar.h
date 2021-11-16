#pragma once

#include <string_view>
#include <variant>
#include <vector>
#include <unordered_set>

#include "tokenizer/Token.h"
#include "ConstexprUtils.h"

namespace dpl {

	class ProductionRule : private std::vector<std::variant<Terminal, std::string_view>> {
	public:

		using symbol_type = value_type;

		constexpr ProductionRule(std::initializer_list<symbol_type> l) : std::vector<symbol_type>(l) { }

		inline constexpr bool isEpsilonProd() const {
			return empty();
		}

		using std::vector<symbol_type>::vector;
		using std::vector<symbol_type>::size;
		using std::vector<symbol_type>::empty;
		using std::vector<symbol_type>::operator[];
		using std::vector<symbol_type>::begin;
		using std::vector<symbol_type>::end;
		using std::vector<symbol_type>::rbegin;
		using std::vector<symbol_type>::rend;
		using std::vector<symbol_type>::push_back;

		friend std::ostream& operator<<(std::ostream& os, const ProductionRule& rule) {
			if (rule.empty()) os << "epsilon";
			else {
				for (auto i = rule.begin(); i != rule.end(); i++) {
					const auto& sym = *i;

					if (i != rule.begin()) {
						os << " ";
					}

					if (const auto* nonterminal = std::get_if<std::string_view>(&sym)) dpl::log::coloredStream(os, 0x0F, *nonterminal);
					else if (const auto* tkn = std::get_if<Terminal>(&sym))
						dpl::log::coloredStream(os, 0x03, tkn->stringify());
				}
			}

			return os;
		}

	};

	class NonterminalRules : private std::vector<ProductionRule> {
	public:

		constexpr NonterminalRules() = default;
		constexpr NonterminalRules(std::string_view n, std::initializer_list<ProductionRule> l) : std::vector<ProductionRule>(l), name(n) { }

		using std::vector<ProductionRule>::size;
		using std::vector<ProductionRule>::operator[];
		using std::vector<ProductionRule>::begin;
		using std::vector<ProductionRule>::end;

		std::string_view name;

		friend std::ostream& operator<<(std::ostream& os, const NonterminalRules& nt) {
			for (const auto& rule : nt) {
				os << "    | " << rule << '\n';
			}
			return os;
		}

	};

	class Grammar : dpl::cc::map<std::string_view, NonterminalRules> {
	public:

		using epsilon_type = std::monostate;
		using terminal_type = Terminal;
		using nonterminal_type = std::string_view;

		constexpr Grammar(std::initializer_list<NonterminalRules> l) : start_symbol(l.begin()->name) {
			std::for_each(l.begin(), l.end(), [&](const auto& e) {
				(*this)[e.name] = e;
			});
		}

		void initialize() {
			calcFirstSets();
			calcFollowSets();
		}

		using dpl::cc::map<std::string_view, NonterminalRules>::size;
		using dpl::cc::map<std::string_view, NonterminalRules>::operator[];
		using dpl::cc::map<std::string_view, NonterminalRules>::begin;
		using dpl::cc::map<std::string_view, NonterminalRules>::end;
		using dpl::cc::map<std::string_view, NonterminalRules>::contains;


	public:

		using firsts_type = dpl::cc::map<nonterminal_type, dpl::cc::set<std::variant<epsilon_type, terminal_type>>>;
		using follows_type = dpl::cc::map<nonterminal_type, dpl::cc::set<terminal_type>>;

		firsts_type firsts;
		follows_type follows;

		std::string start_symbol;
		dpl::cc::set<std::string_view> keywords, symbols;

		friend std::ostream& operator<<(std::ostream& os, Grammar& grammar) {
			for (const auto& [name, nonterminal] : grammar) {
				dpl::log::coloredStream(std::cout, (name == grammar.start_symbol ? 0x02 : 0x0F), name);
				os << " ::=\n";
				os << nonterminal;
				os << "  ;\n\n";
			}
			return os;
		}


	public:

		void calcFirstSets() {
			firsts.reserve(size());
			for (auto& [name, nt] : *this) {
				firsts[name].reserve(nt.size());
				for (auto& rule : nt) {
					if (rule.isEpsilonProd()) {
						firsts[name].insert(epsilon_type{});
					} else if (const terminal_type* t = std::get_if<terminal_type>(&rule[0])) {
						firsts[name].insert(*t);
					}
				}
			}

			bool changed;
			do {
				changed = false;

				for (auto& [name, nt] : (*this)) {
					for (auto& rule : nt) {
						auto size_before = firsts[name].size();

						auto first_star_set = first_star(rule.begin(), rule.end());
						for (const auto& symbol : first_star_set) {
							firsts[name].insert(symbol);
						}

						if (size_before != firsts[name].size()) changed = true;
					}
				}
			} while (changed);
		}

		void calcFollowSets() {
			for (auto& [name, nt] : (*this)) {
				for (auto& rule : nt) {
					if (rule.empty()) continue;

					for (int i = 0; i < rule.size() - 1; i++) {
						if (const auto* n = std::get_if<nonterminal_type>(&rule[i])) {
							if (const auto* t = std::get_if<terminal_type>(&rule[i + 1])) {
								follows[*n].insert(*t);
							}
						}
					}
				}
			}

			follows[start_symbol].insert(Terminal::Type::EndOfFile);

			bool changed;
			do {
				changed = false;

				for (auto& [name, nt] : (*this)) {
					for (auto& rule : nt) {
						if (rule.isEpsilonProd()) continue;

						for (int i = 0; i < rule.size() - 1; i++) {
							if (const auto* v = std::get_if<nonterminal_type>(&rule[i])) {
								auto size_before = follows[*v].size();

								const auto first_of_rest = first_star(std::next(rule.begin(), i + 1), rule.end());
								bool contains_epsilon = false;

								std::for_each(first_of_rest.begin(), first_of_rest.end(), [&](const auto& e) {
									if (std::holds_alternative<terminal_type>(e)) follows[*v].insert(std::get<terminal_type>(e));
									else if (std::holds_alternative<epsilon_type>(e)) contains_epsilon = true;
								});

								if (contains_epsilon) {
									std::for_each(follows[name].begin(), follows[name].end(), [&](const auto& e) {
										follows[*v].insert(e);
									});
								}

								if (size_before != follows[*v].size()) changed = true;
							}
						}
					}
				}

			} while (changed);
		}

	public:

		// #TASK: require constant iterators
		template<class InputIt> requires std::input_iterator<InputIt>
		dpl::cc::set<std::variant<std::monostate, Terminal>> first_star(const InputIt first, const InputIt last) const {
			if (std::distance(first, last) == 0) {
				return { epsilon_type{} };
			}

			if (const auto* v = std::get_if<terminal_type>(&*first)) {
				return { *v };
			}

			if (const auto* v = std::get_if<nonterminal_type>(&*first)) {
				auto result = firsts.at(*v);

				if (firsts.at(*v).contains(epsilon_type{})) {
					result.erase(epsilon_type{});
					auto rest = first_star(std::next(first), last);

					std::for_each(rest.begin(), rest.end(), [&](const auto& e) {
						result.insert(e);
					});
				}

				return result;
			}

			return {};
		}

	};


	struct RuleRef {
	public:

		RuleRef(Grammar& g, std::string_view n, int p) : grammar(&g), name(n), prod(p) { }

		Grammar* grammar;
		std::string_view name;
		int prod;

		const auto& getRule() const {
			return (*grammar)[name][prod];
		}

		friend bool operator==(const RuleRef& lhs, const RuleRef& rhs) {
			return (lhs.name == rhs.name) && (lhs.prod == rhs.prod);
		}

	};
}