#include <dyppyl.h>

int main() {
	using namespace dpl::literals;

	dpl::Grammar grammar {
		"E"nt	|= ("("t, "E"nt, "Op"nt, "E"nt, ")"t)
				| (dpl::Terminal::Type::Number),
		"Op"nt	|= "+"t | "*"t
	};

	dpl::LL1 parser{ grammar, {
		.log_tokenizer = true,
	}};

	std::string input;
	while (true) {
		std::getline(std::cin, input);
		if (input == "quit") break;

		auto [tree, errors] = parser.parse(input);

		if (!errors.empty()) {
			for (const auto& error : errors) {
				std::cout << "Syntax Error: unexpected token " << dpl::streamer{ error.found, 0x03 }
					<< " at position " << dpl::streamer{ error.found.pos, 0x03 } << " of the input!\n"
					"expected any of the following: ";

				dpl::color_cout{0x03} << "\'" << *error.expected.begin() << "\'";
				std::for_each(std::next(error.expected.begin()), error.expected.end(), [](dpl::Terminal terminal) {
					std::cout << ", ";
					dpl::color_cout{ 0x03 } << "\'" << terminal << "\'";
				});
				std::cout << ".\n\n";
			}

			continue;
		}
		
		tree.replace_with(
			dpl::ParseTree{ dpl::RuleRef{"E", 1} },
			[](const dpl::ParseTree& cs) { return dpl::ParseTree{ cs[0].value }; }
		);

		tree.replace_with(
			dpl::ParseTree{ dpl::RuleRef{"E", 0} },
			[](const dpl::ParseTree& cs) { return dpl::ParseTree{ cs[2][0].value, { cs[1], cs[3] }}; }
		);

		std::cout << "\n\nAST:\n" << tree << "\n\n";

		dpl::tree_visit(tree, [](dpl::ParseTree& tree) {
			if (tree.children.size() == 2) {
				long double lhs = std::get<long double>(tree[0].get<dpl::Token>().value);
				long double rhs = std::get<long double>(tree[1].get<dpl::Token>().value);

				if (tree.match({ "+"_sym }))
					tree = dpl::ParseTree{ { dpl::Token{dpl::Token::Type::Number, lhs + rhs }} };
				else if (tree.match({ "*"_sym }))
					tree = dpl::ParseTree{ { dpl::Token{dpl::Token::Type::Number, lhs * rhs }} };
			}
		});

		std::cout << "Result: " << std::get<long double>(tree.get<dpl::Token>().value);
		std::cout << "\n\n\n";
	}
	

	return 0;
}