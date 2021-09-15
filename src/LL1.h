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
			firsts.reserve(g.nonterminals.size());
			for (auto& [name, nt] : g.nonterminals) {
				firsts[name].reserve(nt.size());
				for (auto& rule : nt.getProductions()) {
					if (const Token* t = std::get_if<Token>(&rule[0])) {
						firsts[name].insert(*t);
					}
				}
			}

			bool changed;
			do {
				changed = false;

				for (auto& [name, nt] : g.nonterminals) {
					for (auto& rule : nt.productions) {
						auto size_before = firsts[name].size();

						if (const std::string_view* n = std::get_if<std::string_view>(&rule[0])) {
							if (*n != name) {
								std::for_each(firsts[*n].begin(), firsts[*n].end(), [&](const auto& e) {
									firsts[name].insert(e);
								});
							}
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