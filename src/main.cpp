#include <iostream>
#include <algorithm>

#include "Tokenizer.h"
#include <unordered_map>

// search this solution for "#TASK" to find places where optimizations/refactoring/improvements may be worth implementing

int main() {

	dpl::TokenizerInterface tokenizer;

	tokenizer.tokenizeString("doubdouble");

	std::for_each(tokenizer.base.tokens_out.begin(), tokenizer.base.tokens_out.end(), [](std::string_view s) { std::cout << s << ", "; });

	return 0;
}