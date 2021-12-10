#pragma once

#include <string_view>
#include <variant>
#include <vector>

#include "tokenizer/Token.h"
#include "ConstexprUtils.h"
#include "hybrid/hybrid.hpp"

namespace dpl {

	enum class Assoc { None, Left, Right };

	class ProductionRule : private std::vector<std::variant<Terminal, std::string_view>> {
	public:

		using symbol_type = value_type;
		using std::vector<std::variant<Terminal, std::string_view>>::iterator;
		
		Assoc assoc = Assoc::None;
		short prec = 0;

		constexpr ProductionRule(std::initializer_list<symbol_type> l) : std::vector<symbol_type>(l) { }
		constexpr ProductionRule(std::initializer_list<symbol_type> l, Assoc assoc_, short prec_)
			: std::vector<symbol_type>(l), assoc(assoc_), prec(prec_) { }

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
			for (auto iter = l.begin(); iter != l.end(); iter++) {
				insert(iter->name, *iter);

				// default-assign precedences to terminals
				for (const auto& rule : *iter) {
					auto jter = rule.rbegin();
					while (jter != rule.rend()) {
						if (auto* terminal = std::get_if<terminal_type>(&*jter))
						if (!terminal_precs.contains(*terminal))
							terminal_precs[*terminal] = rule.prec;

						++jter;
					}
				}
			}

			initialize();
		}
		constexpr ~Grammar() = default;

		constexpr void initialize() {
			calcFirstSets();
			calcFollowSets();
		}

		using dpl::cc::map<std::string_view, NonterminalRules>::size;
		using dpl::cc::map<std::string_view, NonterminalRules>::operator[];
		using dpl::cc::map<std::string_view, NonterminalRules>::at;
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
		dpl::cc::map<terminal_type, short> terminal_precs;

		friend std::ostream& operator<<(std::ostream& os, const Grammar& grammar) {
			for (const auto& [name, nonterminal] : grammar) {
				dpl::log::coloredStream(os, (name == grammar.start_symbol ? 0x02 : 0x0F), name);
				os << " ::=\n";
				os << nonterminal;
				os << "    ;\n";
			}
			return os;
		}


	public:

		constexpr void calcFirstSets() {
			if (!std::is_constant_evaluated()) firsts.reserve(size());
			for (const auto& [name, nt] : *this) {
				firsts.insert(name, {});

				if (!std::is_constant_evaluated()) firsts[name].reserve(nt.size());
				// #TASK : change loop to "auto& rule : nt" (why doesn't it work in constexpr??)
				for (int i = 0; i < nt.size(); i++) {
					if (nt[i].isEpsilonProd()) {
						firsts[name].insert(epsilon_type{});
					} else if (const terminal_type* t = std::get_if<terminal_type>(&nt[i][0])) {
						firsts[name].insert(*t);
					}
				}
			}

			bool changed;
			do {
				changed = false;

				for (auto& [name, nt] : *this) {
					// #TASK : change loop to "auto& rule : nt" (why doesn't it work in constexpr??)
					for (int i = 0; i < nt.size(); i++) {
						auto size_before = firsts[name].size();

						auto first_star_set = first_star(nt[i]);
						for (const auto& symbol : first_star_set) {
							firsts[name].insert(symbol);
						}

						if (size_before != firsts[name].size()) changed = true;
					}
				}
			} while (changed);
		}

		constexpr void calcFollowSets() {
			for (auto& [name, nt] : *this) {
				// #TASK : change loop to "auto& rule : nt" (why doesn't it work in constexpr??)
				for (int j = 0; j < nt.size(); j++) {
					auto& rule = nt[j];
					if (rule.empty()) continue;

					for (int i = 0; i < rule.size() - 1; i++) {
						if (const auto* n = std::get_if<nonterminal_type>(&rule[i])) {
							if (const auto* t = std::get_if<terminal_type>(&rule[i + 1])) {
								follows.insert(*n, {});
								follows[*n].insert(*t);
							}
						}
					}
				}
			}

			follows.insert(start_symbol, {});
			follows[start_symbol].insert(Terminal::Type::EndOfFile);

			// #TASK : rewrite this huge ass nested loop
			bool changed;
			do {
				changed = false;

				for (auto& [name, nt] : (*this)) {
					for (int j = 0; j < nt.size(); j++) {
						auto& rule = nt[j];
						if (rule.isEpsilonProd()) continue;

						for (int i = 0; i < rule.size() - 1; i++) {
							if (const auto* v = std::get_if<nonterminal_type>(&rule[i])) {
								auto size_before = follows[*v].size();
								
								dpl::ProductionRule rest_of_rule = {};
								for (int k = i + 1; k < rule.size(); k++) {
									rest_of_rule.push_back(rule[k]);
								}
								auto first_of_rest = first_star(rest_of_rule);
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
		constexpr dpl::cc::set<std::variant<epsilon_type, terminal_type>> first_star(const ProductionRule& rule) const {
			// first* of an epsilon-production is epsilon
			if (rule.isEpsilonProd()) {
				return { epsilon_type{} };
			}

			// first* of a string that begins with a terminal is that terminal
			if (const auto* v = std::get_if<terminal_type>(&rule[0])) {
				return { *v };
			}

			// first* of a string that begins with a nonterminal
			if (const auto* v = std::get_if<nonterminal_type>(&rule[0])) {
				// use the first set of that nonterminal
				auto result = firsts.at(*v);

				// if that nonterminal can begin with epsilon, remove epsilon and add first* of the remaining string
				if (firsts.at(*v).contains(epsilon_type{})) {
					result.erase(epsilon_type{});
					auto rest = first_star({ std::next(rule.begin()), rule.end() });

					std::for_each(rest.begin(), rest.end(), [&](const auto& e) {
						result.insert(e);
					});
				}

				return result;
			}

			return { };
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

		friend bool operator==(const RuleRef& lhs, const RuleRef& rhs) { return (lhs.name == rhs.name) && (lhs.prod == rhs.prod); }
		operator std::string_view() const { return name; }
	};

	inline bool operator==(const RuleRef& lhs, const std::string_view rhs) { return lhs.name == rhs; }
	inline bool operator==(const std::string_view lhs, const RuleRef& rhs) { return rhs == lhs; }
	static_assert(std::equality_comparable_with<RuleRef, std::string_view>);
}