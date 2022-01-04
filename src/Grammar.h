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
}

#include "EDSL.h"

namespace dpl {

	template<typename NonterminalT = std::string_view, typename TerminalT = dpl::Terminal<std::string_view>>
	class ProductionRule {
	public:

		using terminal_type = TerminalT;
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

		template<std::forward_iterator IterT>
		constexpr ProductionRule(IterT first, IterT last) : sentence(first, last) {}

		[[nodiscard]] constexpr bool isEpsilonProd() const { return sentence.empty(); }

		[[nodiscard]] constexpr size_t size() const { return sentence.size(); }
		[[nodiscard]] constexpr bool empty() const { return sentence.empty(); }

		constexpr void clear() { sentence.clear(); }
		constexpr symbol_type& operator[](size_t index) { return sentence[index]; }
		[[nodiscard]] constexpr const symbol_type& operator[](size_t index) const { return sentence[index]; }

		[[nodiscard]] constexpr auto begin() { return sentence.begin(); }
		[[nodiscard]] constexpr auto end() { return sentence.end(); }
		[[nodiscard]] constexpr auto rbegin() { return sentence.rbegin(); }
		[[nodiscard]] constexpr auto rend() { return sentence.rend(); }

		[[nodiscard]] constexpr auto begin() const { return sentence.begin(); }
		[[nodiscard]] constexpr auto end() const { return sentence.end(); }
		[[nodiscard]] constexpr auto rbegin() const { return sentence.rbegin(); }
		[[nodiscard]] constexpr auto rend() const { return sentence.rend(); }

		constexpr void push_back(auto&& elem) { sentence.push_back(elem); }

		friend std::ostream& operator<<(std::ostream& os, const ProductionRule<nonterminal_type>& rule) {
			if (rule.empty()) os << "epsilon";
			else {
				for (auto i = rule.begin(); i != rule.end(); i++) {
					const auto& sym = *i;

					if (i != rule.begin()) os << " ";
					std::cout << dpl::streamer{ sym };
				}
			}

			return os;
		}

	};

	template<typename NonterminalT = std::string_view, typename TerminalT = dpl::Terminal<std::string_view>>
	class NonterminalRules {
	public:

		using terminal_type = TerminalT;
		using nonterminal_type = NonterminalT;

		using symbol_type = std::variant<terminal_type, nonterminal_type>;
		using production_type = ProductionRule<nonterminal_type>;

	private:

		std::vector<production_type> rules;

	public:

		nonterminal_type name;

		constexpr NonterminalRules() = default;
		constexpr NonterminalRules(nonterminal_type n) : name(n) { }
		constexpr NonterminalRules(nonterminal_type n, std::initializer_list<production_type> l)
			: std::vector<production_type>(l), name(n) { }

		[[nodiscard]] constexpr size_t size() const { return rules.size(); }

		constexpr production_type& operator[](size_t index) { return rules[index]; }
		[[nodiscard]] constexpr const production_type& operator[](size_t index) const { return rules[index]; }

		[[nodiscard]] constexpr auto begin() { return rules.begin(); }
		[[nodiscard]] constexpr auto end() { return rules.end(); }
		[[nodiscard]] constexpr auto rbegin() { return rules.rbegin(); }
		[[nodiscard]] constexpr auto rend() { return rules.rend(); }

		[[nodiscard]] constexpr const auto begin() const { return rules.begin(); }
		[[nodiscard]] constexpr const auto end() const { return rules.end(); }
		[[nodiscard]] constexpr const auto rbegin() const { return rules.rbegin(); }
		[[nodiscard]] constexpr const auto rend() const { return rules.rend(); }

		[[nodiscard]] constexpr auto& front() { return rules.front(); }
		[[nodiscard]] constexpr auto& back() { return rules.back(); }

		constexpr void push_back(auto&& elem) { rules.push_back(elem); }

		friend std::ostream& operator<<(std::ostream& os, const NonterminalRules& nt) {
			for (const auto& rule : nt) {
				os << "    | " << rule << '\n';
			}
			return os;
		}

	};

	template<typename TokenT = dpl::Token<>, typename NonterminalT = std::string_view, typename TerminalNameT = std::string_view>
	class Grammar {
	public:

		using epsilon_type = std::monostate;

		using terminal_type = Terminal<TerminalNameT>;
		using token_type = TokenT;
		using nonterminal_type = NonterminalT;

		using prod_type = ProductionRule<nonterminal_type>;
		using ntrule_type = NonterminalRules<nonterminal_type>;

		using firsts_type = std::map<nonterminal_type, hybrid::set<std::variant<epsilon_type, terminal_type>>>;
		using follows_type = std::map<nonterminal_type, hybrid::set<terminal_type>>;

	private:

		std::map<nonterminal_type, ntrule_type> ntrules{};

		firsts_type firsts;
		follows_type follows;

		nonterminal_type start_symbol;
		hybrid::map<terminal_type, short> terminal_precs;

	public:

		template<typename TokenT_>
		friend constexpr std::pair<dpl::Grammar<TokenT_, std::string_view, std::string_view>,
			dpl::Lexicon<char, TokenT_>> decompose_grammar_lit(dpl::GrammarLit<TokenT_> lit);

		constexpr void initialize() {
			calcFirstSets();
			calcFollowSets();
		}

		constexpr ~Grammar() = default;
		constexpr Grammar() = default;

		constexpr Grammar(std::initializer_list<ntrule_type> l) : start_symbol(l.begin()->name) {
			for (auto& ntrule : l) {
				(*this)[ntrule.name].name = ntrule.name;
				for (auto& rule : ntrule) {
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
						// #TASK : still need this block?
						//const auto* terminal = std::get_if<terminal_type>(&sym);
						//if (terminal && terminal->is_wildcard) {
						//	const auto* terminal_str = std::get_if<std::string_view>(&terminal->terminal_value);
						//	if (terminal_str) {
						//		adding_rule.push_back(dpl::Terminal{ Terminal::Type::Symbol, *terminal_str });
						//		continue;
						//	}
						//}

						adding_rule.push_back(sym);
					}

					(*this)[ntrule.name].push_back(adding_rule);
				}
			}

			initialize();
		}


		[[nodiscard]] constexpr size_t size() const { return ntrules.size(); }
		
		constexpr ntrule_type& operator[](const nonterminal_type& index) { return ntrules[index]; }
		[[nodiscard]] constexpr const ntrule_type& operator[](const nonterminal_type& index) const { return ntrules[index]; }
		[[nodiscard]] constexpr const ntrule_type& at(const nonterminal_type& index) const { return ntrules.at(index); }
		
		[[nodiscard]] constexpr auto begin() { return ntrules.begin(); }
		[[nodiscard]] constexpr auto end() { return ntrules.end(); }
		[[nodiscard]] constexpr auto rbegin() { return ntrules.rbegin(); }
		[[nodiscard]] constexpr auto rend() { return ntrules.rend(); }

		[[nodiscard]] constexpr bool contains(const nonterminal_type& index) const { return ntrules.contains(index); }

		friend std::ostream& operator<<(std::ostream& os, const Grammar& grammar) {
			for (const auto& [name, nonterminal] : grammar) {
				dpl::colored_stream(os, (name == grammar.start_symbol ? 0x02 : 0x0F), name);
				os << " ::=\n";
				os << nonterminal;
				os << "    ;\n";
			}
			return os;
		}


	private:

		constexpr void calcFirstSets() {
			for (const auto& [name, nt] : *this) {
				firsts.insert({ name, {} });

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
			for (const auto& [name, nt] : ntrules) {
				// #TASK : change loop to "auto& rule : nt" (why doesn't it work in constexpr??)
				for (int j = 0; j < nt.size(); j++) {
					const auto& rule = nt[j];
					if (rule.empty()) continue;

					for (int i = 0; i < rule.size() - 1; i++) {
						if (const auto* n = std::get_if<nonterminal_type>(&rule[i])) {
							if (const auto* t = std::get_if<terminal_type>(&rule[i + 1])) {
								follows.insert({ *n, {} });
								follows[*n].insert(*t);
							}
						}
					}
				}
			}

			follows.insert({ start_symbol, {} });
			follows[start_symbol].insert(terminal_type{ terminal_type::Type::eof });

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
		constexpr hybrid::set<std::variant<epsilon_type, terminal_type>> first_star(const prod_type& rule) const {
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

	template<typename GrammarT = dpl::Grammar<>>
	struct RuleRef {
	public:
		using grammar_type = GrammarT;
		using nonterminal_type = grammar_type::nonterminal_type;

	private:
		grammar_type* grammar;
		nonterminal_type name;
		size_t prod;

	public:

		RuleRef(grammar_type& g, nonterminal_type n, int p) : grammar(&g), name(n), prod(p) { }
		RuleRef(nonterminal_type n, int p) : grammar(nullptr), name(n), prod(p) { }

		const grammar_type* get_grammar() const { return grammar; }
		const nonterminal_type get_name() const { return name; }
		const size_t get_prod() const { return prod; }

		const auto& getRule() const { return (*grammar)[name][prod]; }

		friend bool operator==(const RuleRef& lhs, const RuleRef& rhs) { return (lhs.name == rhs.name) && (lhs.prod == rhs.prod); }
		operator nonterminal_type() const { return name; }
	};

	template<typename GrammarT>
	inline bool operator==(const RuleRef<GrammarT>& lhs, const typename GrammarT::nonterminal_type rhs) { return lhs.name == rhs; }

	template<typename GrammarT>
	inline bool operator==(const typename GrammarT::nonterminal_type lhs, const RuleRef<GrammarT>& rhs) { return rhs == lhs; }

	static_assert(std::equality_comparable_with<RuleRef<>, std::string_view>);
}


// grammar literal destructuring
namespace dpl {
	template<typename TokenT>
	constexpr std::pair<dpl::Grammar<TokenT, std::string_view, std::string_view>,
		dpl::Lexicon<char, TokenT>> decompose_grammar_lit(dpl::GrammarLit<TokenT> lit) {

		using grammar_type = dpl::Grammar<TokenT, std::string_view, std::string_view>;
		using lexicon_type = dpl::Lexicon<char, TokenT>;

		grammar_type grammar;
		lexicon_type lexicon;

		// add all explicitly-defined lexemes
		for (const auto& lexeme : lit.lexemes) {
			if (lexeme.discard) {
				if (lexicon.discard_regex) throw std::exception{ "discard redefinition" };
				lexicon.discard_regex = lexeme.lex.regex;
				continue;
			}

			if (lexicon.contains(lexeme.name)) throw std::exception{ "terminal redefinition" };
			lexicon.insert({ lexeme.name, lexeme.lex });
		}

		// add all grammar rules
		for (const auto& ntrule : lit.rules) {
			if (grammar.contains(ntrule.name)) throw std::exception{ "nonterminal redefinition" };

			if (grammar.ntrules.empty()) grammar.start_symbol = ntrule.name;
			grammar.ntrules.insert({ ntrule.name, typename grammar_type::ntrule_type{ ntrule.name } });
			for (const auto& prod : ntrule.prods) {
				grammar.ntrules[ntrule.name].push_back(typename grammar_type::prod_type{});
				for (const auto& sym : prod.sentence) {

					// add nonterminals as themselves, terminals from the lexicon
					if (const auto* nonterminal = std::get_if<dpl::NonterminalLit>(&sym)) {
						grammar.ntrules[ntrule.name].back().push_back(*nonterminal);
					} else if (const auto* terminal = std::get_if<dpl::TerminalLit>(&sym)) {

						if (lexicon.contains(*terminal)) {
							grammar.ntrules[ntrule.name].back().push_back(typename grammar_type::terminal_type{ *terminal });
						} else {
							// if terminal not in lexicon, add its raw string value
							grammar.ntrules[ntrule.name].back().push_back(typename grammar_type::terminal_type{ *terminal });
							lexicon.insert({ *terminal, dpl::Lexeme{ dpl::match{ *terminal } } });
						}

					} // exhaustive
				}
			}
		}

		// calculate firsts and follows
		grammar.initialize();

		return std::pair{ grammar, lexicon };
	}
}

namespace std {
	template<typename TokenT>
	struct tuple_size<dpl::GrammarLit<TokenT>> : integral_constant<size_t, 2> {};

	template<typename TokenT>
	struct tuple_element<0, dpl::GrammarLit<TokenT>> {
		using type = dpl::Grammar<TokenT, std::string_view, std::string_view>;
	};

	template<typename TokenT>
	struct tuple_element<1, dpl::GrammarLit<TokenT>> {
		using type = dpl::Lexicon<char, TokenT>;
	};

	template<size_t Index, typename TokenT>
	std::tuple_element_t<Index, dpl::GrammarLit<TokenT>> get(const dpl::GrammarLit<TokenT>& g) {
		auto decomposed = dpl::decompose_grammar_lit(g);

		if constexpr (Index == 0) {
			auto copy_grammar = decomposed.first;

			return decomposed.first;
		}
		if constexpr (Index == 1) {
			return decomposed.second;
		}
	}
}