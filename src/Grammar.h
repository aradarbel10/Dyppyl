#pragma once

#include <string_view>
#include <variant>
#include <vector>

#include "tokenizer/Token.h"
#include "ConstexprUtils.h"
#include "hybrid/hybrid.hpp"
#include "Lexical.h"

namespace dpl {

	enum class Assoc { None, Left, Right };

	template<typename AtomT = char, typename NonterminalT = std::string_view>
	class ProductionRule {
	public:

		using atom_type = AtomT;
		using terminal_type = Terminal;
		using nonterminal_type = NonterminalT;

		using symbol_type = std::variant<terminal_type, nonterminal_type>;

	private:

		std::vector<symbol_type> sentence;

	public:
		
		Assoc assoc = Assoc::None;
		short prec = 0;

		constexpr ProductionRule(std::initializer_list<symbol_type> l) : sentence(l) { }
		constexpr ProductionRule(std::initializer_list<symbol_type> l, Assoc assoc_, short prec_)
			: sentence(l), assoc(assoc_), prec(prec_) { }

		[[nodiscard]] constexpr bool isEpsilonProd() const { return sentence.empty(); }

		[[nodiscard]] constexpr size_t size() const { return sentence.size(); }
		[[nodiscard]] constexpr bool empty() const { return sentence.empty(); }

		constexpr void clear() { sentence.clear(); }
		constexpr symbol_type& operator[](size_t index) { return sentence[index]; }
		[[nodiscard]] constexpr const symbol_type& operator[](size_t index) const { return sentence[index]; }

		[[nodiscard]] constexpr auto begin() { sentence.begin(); }
		[[nodiscard]] constexpr auto end() { sentence.end(); }
		[[nodiscard]] constexpr auto rbegin() { sentence.rbegin(); }
		[[nodiscard]] constexpr auto rend() { sentence.rend(); }

		constexpr void push_back(auto&& elem) { sentence.push_back(elem); }

		friend std::ostream& operator<<(std::ostream& os, const ProductionRule& rule) {
			if (rule.empty()) os << "epsilon";
			else {
				for (auto i = rule.begin(); i != rule.end(); i++) {
					const auto& sym = *i;

					if (i != rule.begin()) {
						os << " ";
					}

					if (const auto* nonterminal = std::get_if<std::string_view>(&sym)) dpl::colored_stream(os, 0x0F, *nonterminal);
					else if (const auto* tkn = std::get_if<Terminal>(&sym))
						dpl::colored_stream(os, 0x03, tkn->stringify());
				}
			}

			return os;
		}

	};

	/*
	class NonterminalRules : private std::vector<ProductionRule<>> {
	public:

		constexpr NonterminalRules() = default;
		constexpr NonterminalRules(std::string_view n, std::initializer_list<ProductionRule<>> l) : std::vector<ProductionRule>(l), name(n) { }

		using std::vector<ProductionRule>::size;
		using std::vector<ProductionRule>::operator[];
		using std::vector<ProductionRule>::begin;
		using std::vector<ProductionRule>::end;
		using std::vector<ProductionRule>::push_back;

		std::string_view name;

		friend std::ostream& operator<<(std::ostream& os, const NonterminalRules& nt) {
			for (const auto& rule : nt) {
				os << "    | " << rule << '\n';
			}
			return os;
		}

	};

	template<typename AtomT = char>
	class Grammar : dpl::cc::map<std::string_view, NonterminalRules> {
	public:

		using epsilon_type = std::monostate;
		using terminal_type = Terminal;
		using nonterminal_type = std::string_view;

		constexpr Grammar(std::initializer_list<NonterminalRules> l) : start_symbol(l.begin()->name) {
			for (const auto& ntrule : l) {
				(*this)[ntrule.name].name = ntrule.name;
				for (const auto& rule : ntrule) {
					ProductionRule adding_rule = rule;
					adding_rule.clear();

					// default-assign precedences to terminals
					auto iter = rule.rbegin();
					while (iter != rule.rend()) {
						if (auto* terminal = std::get_if<terminal_type>(&*iter))
						if (!terminal_precs.contains(*terminal))
							terminal_precs[*terminal] = rule.prec;

						++iter;
					}

					// preprocess grammar
					for (const auto& sym : rule) {
						const auto* terminal = std::get_if<terminal_type>(&sym);
						if (terminal && terminal->type == Terminal::Type::Unknown) {
							const auto* terminal_str = std::get_if<std::string_view>(&terminal->terminal_value);
							if (terminal_str) {
								adding_rule.push_back(dpl::Terminal{ Terminal::Type::Symbol, *terminal_str });
								continue;
							}
						}

						adding_rule.push_back(sym);
					}

					(*this)[ntrule.name].push_back(adding_rule);
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
		dpl::cc::map<terminal_type, short> terminal_precs;

		dpl::Lexicon<AtomT> lexicon;

		friend std::ostream& operator<<(std::ostream& os, const Grammar& grammar) {
			for (const auto& [name, nonterminal] : grammar) {
				dpl::colored_stream(os, (name == grammar.start_symbol ? 0x02 : 0x0F), name);
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

	template<typename AtomT = char>
	struct RuleRef {
	public:

		RuleRef(Grammar<AtomT>& g, std::string_view n, int p) : grammar(&g), name(n), prod(p) { }
		RuleRef(std::string_view n, int p) : grammar(nullptr), name(n), prod(p) { }

		Grammar<AtomT>* grammar;
		std::string_view name;
		int prod;

		const auto& getRule() const {
			return (*grammar)[name][prod];
		}

		friend bool operator==(const RuleRef& lhs, const RuleRef& rhs) { return (lhs.name == rhs.name) && (lhs.prod == rhs.prod); }
		operator std::string_view() const { return name; }
	};

	template<typename AtomT>
	inline bool operator==(const RuleRef<AtomT>& lhs, const std::string_view rhs) { return lhs.name == rhs; }

	template<typename AtomT>
	inline bool operator==(const std::string_view lhs, const RuleRef<AtomT>& rhs) { return rhs == lhs; }

	static_assert(std::equality_comparable_with<RuleRef<>, std::string_view>);

	*/
}

#include "EDSL.h"