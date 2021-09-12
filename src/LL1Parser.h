#pragma once

#include <string_view>
#include <variant>
#include <vector>

#include "Token.h"

namespace dpl {

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT>&& std::is_enum_v<SymT>
	class Nonterminal;

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT>&& std::is_enum_v<SymT>
	class GrammarAtom {

		using Token = Token<KwdT, SymT>;
		using Nonterminal = Nonterminal<KwdT, SymT>;

		std::variant<Token, Nonterminal> val;

	public:

		GrammarAtom() {

		}

		bool isTerminal() {
			return std::holds_alternative<Token>(val);
		}

	};

	template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT> && std::is_enum_v<SymT>
	class Nonterminal {
	public:

		using GrammarAtom = GrammarAtom<KwdT, SymT>;

		std::vector<GrammarAtom> definition;

		auto& expand() {
			return &definition;
		}
	};
}