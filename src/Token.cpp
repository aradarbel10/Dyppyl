#include "Token.h"

#include <string>

namespace dpl {
	
	std::map<std::string_view, size_t> Token::keywords;
	std::map<std::string_view, size_t> Token::symbols;
	
	/*constexpr std::partial_ordering operator<=>(const Token& lhs, const Token& rhs) {
		return std::tie(lhs.type, lhs.value, lhs.pos) <=> std::tie(rhs.type, rhs.value, rhs.pos);
	}

	constexpr bool operator==(const Token& lhs, const Token& rhs) {
		if (lhs.type != rhs.type) return false;
		if (std::holds_alternative<std::monostate>(lhs.value) || std::holds_alternative<std::monostate>(rhs.value)) {
			return true;
		}
		return lhs.value == rhs.value;
	}*/

	std::ostream& operator<<(std::ostream& os, const Token& t) {
		os << "[" << magic_enum::enum_name(t.type) << ", ";

		std::cout << dpl::log::streamer{ t.value };

		os << "]";
		return os;
	}

	std::string_view Token::symbolByIndex(size_t index) {
		for (const auto& [key, val] : symbols) {
			if (index == val) return key;
		}
		return "";
	}

	std::string_view Token::keywordByIndex(size_t index) {
		for (const auto& [key, val] : keywords) {
			if (index == val) return key;
		}
		return "";
	}

	/*constexpr std::string Token::stringify() const {
		std::string type_name(magic_enum::enum_name(type));
		if (std::holds_alternative<std::monostate>(value)) return type_name;

		if (type == Type::Symbol) {
			return std::string{ symbolByIndex(std::get<size_t>(value)) };
		} else if (type == Type::Keyword) {
			return std::string{ keywordByIndex(std::get<size_t>(value)) };
		} else if (type == Type::Identifier) {
			return std::get<std::string>(value);
		} else if (type == Type::EndOfFile) {
			return "EOF";
		} else {

			if (const auto* str = std::get_if<std::string>(&value)) return type_name + " " + *str;
			else if (const auto* dbl = std::get_if<long double>(&value)) return type_name + " " + std::to_string(*dbl);
			else return type_name;
		}
	}*/
}