#pragma once

#include <string_view>
#include <variant>
#include <vector>
#include <unordered_set>

#include "tokenizer/Token.h"

namespace dpl {

	class ProductionRule : private std::vector<std::variant<Token, std::string_view>> {
	public:

		using Atom = std::variant<Token, std::string_view>;

		ProductionRule(std::initializer_list<Atom> l) : std::vector<Atom>(l) { }

		inline constexpr bool isEpsilonProd() const {
			return empty();
		}

		using std::vector<Atom>::vector;
		using std::vector<Atom>::size;
		using std::vector<Atom>::empty;
		using std::vector<Atom>::operator[];
		using std::vector<Atom>::begin;
		using std::vector<Atom>::end;
		using std::vector<Atom>::rbegin;
		using std::vector<Atom>::rend;
		using std::vector<Atom>::push_back;

		friend std::ostream& operator<<(std::ostream& os, const ProductionRule& rule) {
			if (rule.empty()) os << "epsilon";
			else for (const auto& atom : rule) {
				if (const auto* nonterminal = std::get_if<std::string_view>(&atom)) dpl::log::coloredStream(os, 0x0F, *nonterminal);
				else if (const auto* tkn = std::get_if<Token>(&atom))
					dpl::log::coloredStream(os, 0x03, tkn->stringify());

				os << " ";
			}

			return os;
		}

	};

	class Nonterminal : private std::vector<ProductionRule> {
	public:

		Nonterminal() = default;
		Nonterminal(std::string_view n, std::initializer_list<ProductionRule> l) : std::vector<ProductionRule>(l), name(n) { }

		using std::vector<ProductionRule>::size;
		using std::vector<ProductionRule>::operator[];
		using std::vector<ProductionRule>::begin;
		using std::vector<ProductionRule>::end;

		std::string_view name;

		friend std::ostream& operator<<(std::ostream& os, const Nonterminal& nt) {
			for (const auto& rule : nt) {
				os << "    | " << rule << '\n';
			}
			return os;
		}

	};

	class Grammar : std::unordered_map<std::string_view, Nonterminal> {
	public:

		Grammar(std::initializer_list<Nonterminal> l) : start_symbol((*l.begin()).name) {
			std::for_each(l.begin(), l.end(), [&](const auto& e) {
				(*this)[e.name] = e;
			});
		}

		void initialize() {
			calcFirstSets();
			calcFollowSets();
		}

		using std::unordered_map <std::string_view, Nonterminal>::size;
		using std::unordered_map <std::string_view, Nonterminal>::operator[];
		using std::unordered_map <std::string_view, Nonterminal>::begin;
		using std::unordered_map <std::string_view, Nonterminal>::end;
		using std::unordered_map <std::string_view, Nonterminal>::contains;


	public:

		std::unordered_map<std::string_view, std::unordered_set<std::variant<std::monostate, Token>>> firsts;
		std::unordered_map<std::string_view, std::unordered_set<Token>> follows;

		std::string start_symbol;

		friend std::ostream& operator<<(std::ostream& os, const Grammar& grammar) {
			for (const auto& [name, nonterminal] : grammar) {
				dpl::log::coloredStream(std::cout, (name == grammar.start_symbol ? 0x02 : 0x0F), name);
				os << " ::=\n";
				os << nonterminal;
				os << "  ;\n\n";
			}
			return os;
		}

	private:

		void calcFirstSets() {
			firsts.reserve(size());
			for (auto& [name, nt] : (*this)) {
				firsts[name].reserve(nt.size());
				for (auto& rule : nt) {
					if (rule.empty()) {
						firsts[name].insert(std::monostate());
					} else if (const Token* t = std::get_if<Token>(&rule[0])) {
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
						if (const auto* n = std::get_if<std::string_view>(&rule[i])) {
							if (const auto* t = std::get_if<Token>(&rule[i + 1])) {
								follows[*n].insert(*t);
							}
						}
					}
				}
			}

			follows[start_symbol].insert(Token::Type::EndOfFile);

			bool changed;
			do {
				changed = false;

				for (auto& [name, nt] : (*this)) {
					for (auto& rule : nt) {
						if (rule.empty()) continue;

						for (int i = 0; i < rule.size() - 1; i++) {
							if (const auto* v = std::get_if<std::string_view>(&rule[i])) {
								auto size_before = follows[*v].size();

								const auto first_of_rest = first_star(std::next(rule.begin(), i + 1), rule.end());
								bool contains_epsilon = false;

								std::for_each(first_of_rest.begin(), first_of_rest.end(), [&](const auto& e) {
									if (std::holds_alternative<Token>(e)) follows[*v].insert(std::get<Token>(e));
									else if (std::holds_alternative<std::monostate>(e)) contains_epsilon = true;
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

		// TASK: require constant iterators
		template<class InputIt> requires std::input_iterator<InputIt>
		std::unordered_set<std::variant<std::monostate, Token>> first_star(InputIt first, InputIt last) {
			if (std::distance(first, last) == 0) {
				return { std::monostate() };
			}

			if (const auto* v = std::get_if<Token>(&(*first))) {
				return { *v };
			}

			if (const auto* v = std::get_if<std::string_view>(&(*first))) {
				auto result = firsts[*v];

				if (firsts[*v].contains(std::monostate())) {
					result.erase(std::monostate());
					auto rest = first_star(std::next(first), last);

					std::for_each(rest.begin(), rest.end(), [&](const auto& e) {
						result.insert(e);
					});

					return result;
				} else {
					return result;
				}
			}
		}

	};
}