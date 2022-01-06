#include <dyppyl.h>

int main() {
	using namespace dpl::literals;

	auto [grammar, lexicon] = ( // context free grammar
		dpl::discard |= dpl::Lexeme{ dpl::kleene{dpl::whitespace} },

		"num"t  |= dpl::Lexeme{ dpl::some{dpl::digit}, [](std::string_view str) -> long double { return dpl::from_string<int>(str); } },

		"E"nt	|= ("("t, "E"nt, "Op"nt, "E"nt, ")"t)
				|  ("num"t),
		"Op"nt	|= "+"t | "*"t
	);

	dpl::LR0 parser{ grammar, lexicon, {
		.error_mode = dpl::ErrorMode::Panic
	}};

	while (true) {

		std::string input;
		std::getline(std::cin, input);
		if (input == "quit") break;

		auto [tree, errors] = parser.parse(input); // parsing is super easy

		if (!errors.empty()) { // handle errors.
			for (const auto& error : errors) {
				std::cout << "Syntax Error: unexpected token " << dpl::streamer{ error.found, 0x03 }
					<< " at position " << dpl::streamer{ error.found.pos, 0x03 } << " of the input!\nexpected any of the following: ";

				for (auto iter = error.expected.begin(); iter != error.expected.end(); iter++) {
					if (iter != error.expected.begin()) std::cout << ", ";
					dpl::color_cout{ 0x03 } << "\'" << *iter << "\'";
				}
				std::cout << ".\n\n";
			}

			continue;
		}
		

		tree.replace_with(
			dpl::ParseTree<>{ dpl::RuleRef{"E", 1} },
			[](const auto& cs) { return dpl::ParseTree<>{ cs[0].value }; }
		);

		tree.replace_with(
			dpl::ParseTree<>{ dpl::RuleRef{"E", 0} },
			[](const auto& cs) { return dpl::ParseTree<>{ cs[2][0].value, { cs[1], cs[3] }}; }
		);

		std::cout << "\n\nAST:\n" << tree << "\n\n";

		dpl::tree_visit(tree, [](dpl::ParseTree<>& tree) {
			if (tree.children.size() == 2) {
				long double lhs = std::get<long double>(std::get<dpl::Token<>>(tree[0].value).value);
				long double rhs = std::get<long double>(std::get<dpl::Token<>>(tree[1].value).value);

				if (tree.match({ "+" }))
					tree = dpl::ParseTree<>{ dpl::Token<>{ "num", lhs + rhs } };
				else if (tree.match({ "*" }))
					tree = dpl::ParseTree<>{ dpl::Token<>{ "num", lhs * rhs } };
			}
		});

		std::cout << "Result: " << std::get<long double>(std::get<dpl::Token<>>(tree.value).value);
		std::cout << "\n\n\n";
	}

	return 0;
}