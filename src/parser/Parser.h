#pragma once

#include <string_view>

namespace dpl{
	class Parser {
	public:
		
		virtual std::string_view getParserName() = 0;

	};
}