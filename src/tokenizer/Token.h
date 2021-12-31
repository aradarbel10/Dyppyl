#pragma once

#include <type_traits>

#include <variant>
#include <functional>
#include <unordered_map>
#include <string>
#include <map>
#include <string_view>
#include <variant>
#include <any>

#include <iostream>
#include "magic_enum/magic_enum.hpp"
#include "../Logger.h"
#include "../Regex.h"

namespace dpl {

	struct Terminal {
		using terminal_id_type = size_t;

		terminal_id_type id = -1;
		bool is_wildcard = false;

		constexpr friend auto operator<=>(const Terminal& lhs, const Terminal& rhs) {
			if (lhs == rhs) return std::strong_ordering::equal;
			else return std::tie(lhs.id) <=> std::tie(rhs.id);
		}

		constexpr friend bool operator==(const Terminal& lhs, const Terminal& rhs) {
			if (lhs.is_wildcard || rhs.is_wildcard) return true;
			else return lhs.id == rhs.id;
		}

		friend std::ostream& operator<<(std::ostream& os, const Terminal& t) {
			os << t.stringify();
			return os;
		}

		std::string stringify() const {
			if (is_wildcard) return std::string{ "[wildcard]" };
			else if (id == -1) return std::string{ "[EOF]" };
			else {
				std::ostringstream str;
				str << '[' << dpl::streamer{ id } << ']';
				return std::string{ str.str() };
			}
		}

		static constexpr Terminal eof() {
			return Terminal{ std::numeric_limits<terminal_id_type>::max() };
		}

		constexpr Terminal() : is_wildcard(true) {}
		constexpr Terminal(terminal_id_type id_) : id(id_) { }

	};

	template<typename T = std::variant<std::monostate, std::string_view, std::string, long double>>
	struct Token : public Terminal {

		using value_type = T;

		value_type value;
		file_pos_t pos;

		friend constexpr auto operator<=>(const Token& lhs, const Token& rhs) {
			if (lhs.is_wildcard || rhs.is_wildcard) return std::partial_ordering::equivalent;
			return std::tie(lhs.type, lhs.value, lhs.pos) <=> std::tie(rhs.type, rhs.value, rhs.pos);
		}

		friend constexpr bool operator==(const Token& lhs, const Token& rhs) {
			if (lhs.is_wildcard || rhs.is_wildcard) return true;
			if (lhs.type != rhs.type) return false;
			if (lhs.terminal_value != rhs.terminal_value) return false;
			return lhs.value == rhs.value;
		}

		friend std::ostream& operator<<(std::ostream& os, const Token& t) {
			os << t.stringify();
			return os;
		}

		constexpr std::string stringify() const {
			if (is_wildcard) return std::string{ "[wildcard]" };
			else if (id == -1) return std::string{ "[EOF]" };
			else {
				std::ostringstream str;
				str << '[' << dpl::streamer{ id } << ", " << dpl::streamer{ value } << ']';
				return std::string{ str.str() };
			}
		}

		Token() = default;
		Token(Terminal t) : Terminal(t) {}
		Token(terminal_id_type id_) : Terminal(id_) {}
		Token(terminal_id_type id_, value_type v) : Terminal(id_), value(v) {}
	};
}

namespace std {
	template<typename T>
		requires requires { std::hash<T>{}; }
	class hash<dpl::Token<T>> {
	public:
		//credit to boost::hash_combine
		std::size_t operator()(dpl::Token<T> const& t) const noexcept {
			size_t intermediate = std::hash<dpl::Terminal>{}(static_cast<dpl::Terminal>(t));
			intermediate ^= std::hash<typename dpl::Token<T>::value_type>{}(t.value)
				+ 0x9e3779b9 + (intermediate << 6) + (intermediate >> 2);
			return intermediate;
		}
	};

	template<> class hash<dpl::Terminal> {
	public:
		std::size_t operator()(dpl::Terminal const& t) const noexcept {
			return std::hash<size_t>{}(t.id) ^ std::hash<bool>{}(t.is_wildcard);
		}
	};
}