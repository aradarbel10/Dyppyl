#include <iostream>
#include <algorithm>

#include "Tokenizer.h"
#include <unordered_map>

// search this solution for "#TASK" to find places where optimizations/refactoring/improvements may be worth implementing

int main() {

	const std::string test_code = "int main(int argc, char **argv) {\n\t std::cout << \"Num Of Args:\" << argc << \"\\n\";\n\t return -3.14159;\n }";

	dpl::TokenizerInterface tokenizer;
	tokenizer.tokenizeString(test_code);
	std::for_each(tokenizer.base.tokens_out.begin(), tokenizer.base.tokens_out.end(), [](const auto& s) { std::cout << s << "\n"; });

	return 0;
}