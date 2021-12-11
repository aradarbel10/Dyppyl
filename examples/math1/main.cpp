#include <dyppyl.h>

int main() {
	using namespace dpl::literals;

	dpl::Grammar grammar {
		"E"nt	|= ("("t, "E"nt, "Op"nt, "E"nt, ")"t)
				| (dpl::Terminal::Type::Number),
		"Op"nt	|= "+"t | "*"t
	};
	dpl::LL1 parser{ grammar };

	std::string input;
	while (true) {
		std::getline(std::cin, input);
		if (input == "quit") break;

		auto tree = parser.parse_str(input);

		tree.replace_with(
			dpl::ParseTree{ "E", {{ "("_sym }, {}, { "Op" }, {}, {")"_sym}} },
			[](const std::vector<dpl::ParseTree>& cs) {
				return dpl::ParseTree{ cs[1][0].value, {
					(cs[0].match({"E"}) ? cs[0][0].value : cs[0]),
					(cs[2].match({"E"}) ? cs[2][0].value : cs[2])
				}};
			},
			dpl::TraverseOrder::BottomUp
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