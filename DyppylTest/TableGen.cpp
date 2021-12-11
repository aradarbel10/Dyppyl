#include "catch2/catch.hpp"

#include "../src/Grammar.h"
#include "../src/TextStream.h"
#include "../src/parser/LL1.h"

#include <string_view>

#include "hybrid/hybrid.hpp"

using namespace std::literals::string_view_literals;
using namespace dpl::literals;

constexpr auto gen_table() {
	dpl::Grammar grammar{
		{ "E", {
			{ "int"_kwd },
			{ "("_sym, "E", "Op", "E", ")"_sym }
		}},
		{ "Op", {
			{ "+"_sym },
			{ "*"_sym }
		}}
	};

	auto table = dpl::LLTable{ grammar };

	hybrid::map<std::pair<dpl::LLTable::terminal_type, dpl::LLTable::nonterminal_type>, int> hybrid_table;
	for (const auto& [pair, val] : table) {
		hybrid_table.insert(pair, val);
	}

	return hybrid_table;
}

TEST_CASE("TableGen", "[constexpr]") {
	std::cout << " ===== TableGen [constexpr] =============================\n";

	//constexpr auto table = hybrid_compute(gen_table, );

	//for (const auto& [pair, val] : table) {
	//	std::cout << pair.first.stringify() << ", " << pair.second << ": " << val << '\n';
	//}
}