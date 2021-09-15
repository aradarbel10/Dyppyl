#pragma once

#include "Grammar.h"

#include <unordered_map>
#include <algorithm>

namespace dpl{
	template<typename KwdT, typename SymT>
	class LL1 {
	public:

		using Token = Token<KwdT, SymT>;
		using ProductionRule = ProductionRule<KwdT, SymT>;
		using Nonterminal = Nonterminal<KwdT, SymT>;
		using Grammar = Grammar<KwdT, SymT>;

		LL1(Grammar& g) : grammar(g) {

			calcFirstSets();

		}

		void calcFirstSets() {
			firsts.reserve(grammar.nonterminals.size());
			for (auto& [name, nt] : grammar.nonterminals) {
				firsts[name].reserve(nt.size());
				for (auto& rule : nt.getProductions()) {
					if (const Token* t = std::get_if<Token>(&rule[0])) {
						firsts[name].insert(*t);
					}

					if (rule.size() == 1) {
						if (std::holds_alternative<std::monostate>(rule[0])) {
							firsts[name].insert(std::monostate());
						}
					}
				}
			}

			bool changed;
			do {
				changed = false;

				for (auto& [name, nt] : grammar.nonterminals) {
					for (auto& rule : nt.productions) {
						auto size_before = firsts[name].size();

						int i = 0;
						for (i = 0; i < rule.size(); i++) {
							if (const auto* v = std::get_if<Token>(&rule[i])) {
								firsts[name].insert(*v);
								break;
							} else if (const auto* v = std::get_if<std::string_view>(&rule[i])) {
								if (!firsts[*v].contains(std::monostate())) {
									std::for_each(firsts[*v].begin(), firsts[*v].end(), [&](const auto& e) {
										firsts[name].insert(e);
									});
									break;
								}
							}
						}
						
						if (i == rule.size()) {
							firsts[name].insert(std::monostate());
						}

						if (size_before != firsts[name].size()) changed = true;
					}
				}
			} while (changed);
		}

	private:

		Grammar& grammar;
		std::unordered_map<std::string_view, std::unordered_set<std::variant<std::monostate, Token>>> firsts;
	};
}