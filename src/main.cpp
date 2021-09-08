#include <iostream>

#include "Tokenizer.h"
#include <unordered_map>

int main() {


	dpl::TokenizerInterface tokenizer;

	tokenizer.tokenizeString("abc//defg:\n\t/*123+456*/moretext");
	std::cout << tokenizer.base.lexeme_buff;

	return 0;
}