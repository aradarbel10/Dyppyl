#include <iostream>

#include "Tokenizer.h"
#include <unordered_map>

int main() {


	dpl::TokenizerInterface tokenizer;

	tokenizer.tokenizeString("abcdefg:\n\t123+456");

	for (const auto& t : result) {
		std::cout << t << '\n';
	}

	return 0;
}