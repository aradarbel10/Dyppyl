#pragma once

#include <type_traits>

#include <variant>
#include <functional>
#include <unordered_map>
#include <string>
#include <map>
#include <string_view>
#include <variant>
#include <iostream>

#include <iostream>
#include "../Logger.h"
#include "../Regex.h"

namespace dpl {

	template<typename NameT = std::string_view>
	struct Terminal {
		using name_type = NameT;

		enum class Type { value, eof, wildcard, unknown } type = Type::unknown;
		name_type name{};

		constexpr Terminal() = default;
		constexpr Terminal(Type t_) : type(t_) {};
		constexpr Terminal(name_type name_) : type(Type::value), name(name_) {}

		constexpr bool is_wildcard() const { return type == Type::wildcard; }
		constexpr bool is_eof() const { return type == Type::eof; }
		constexpr bool is_unknown() const{ return type == Type::unknown; }

		constexpr friend auto operator<=>(const Terminal<name_type>& lhs, const Terminal<name_type>& rhs) {
			if (lhs == rhs) return std::strong_ordering::equal;
			else return std::tie(lhs.name) <=> std::tie(rhs.name);
		}

		constexpr friend bool operator==(const Terminal<name_type>& lhs, const Terminal<name_type>& rhs) {
			if (lhs.is_wildcard() || rhs.is_wildcard()) return true;
			else return lhs.name == rhs.name;
		}

		constexpr friend bool operator==(const Terminal<name_type>& lhs, const name_type& rhs) {
			if (lhs.is_wildcard()) return true;
			else return lhs.name == rhs;
		}
		constexpr friend bool operator==(const name_type& lhs, const Terminal<name_type>& rhs) { return rhs == lhs; }

		friend std::ostream& operator<<(std::ostream& os, const Terminal& t) {
			os << t.stringify();
			return os;
		}

		std::string stringify() const {
			if (is_wildcard()) return std::string{ "[wildcard]" };
			else if (is_eof()) return std::string{ "[EOF]" };
			else if (is_unknown()) return std::string{ "[unknown]" };
			else {
				std::ostringstream str;
				str << '[' << dpl::streamer{ name } << ']';
				return std::string{ str.str() };
			}
		}

	};

	template<typename T = std::variant<std::monostate, std::string_view, long double>, typename TerminalNameT = std::string_view>
	struct Token : public Terminal<TerminalNameT> {

		using value_type = T;
		using name_type = TerminalNameT;
		
		using terminal_type = Terminal<name_type>;
		using token_type = Token<value_type, name_type>;

		value_type value;
		file_pos_t pos{};


		Token() = default;
		Token(const Token<value_type, name_type>& other) : terminal_type(other), value(other.value) {}
		Token(terminal_type t) : terminal_type(t) {}
		Token(name_type name_, value_type v) : terminal_type(name_), value(v) {}

		friend constexpr auto operator<=>(const token_type& lhs, const token_type& rhs) {
			if (lhs.is_wildcard() || rhs.is_wildcard()) return std::partial_ordering::equivalent;
			return std::tie(lhs.name, lhs.type, lhs.value, lhs.pos) <=> std::tie(rhs.name, rhs.type, rhs.value, rhs.pos);
		}

		friend constexpr bool operator==(const token_type& lhs, const token_type& rhs) {
			if (lhs.is_wildcard() || rhs.is_wildcard()) return true;
			return (lhs.name == rhs.name) && (lhs.value == rhs.value);
		}

		friend std::ostream& operator<<(std::ostream& os, const Token& t) {
			os << t.stringify();
			return os;
		}

		constexpr std::string stringify() const {
			if (this->is_wildcard()) return std::string{ "[wildcard]" };
			else if (this->is_eof()) return std::string{ "[EOF]" };
			else if (this->is_unknown()) return std::string{ "[unknown]" };
			else {
				std::ostringstream str;
				str << '[' << dpl::streamer{ this->name } << ", " << dpl::streamer{ value } << ']';
				return std::string{ str.str() };
			}
		}
	};
}

namespace std {
	template<typename T, typename S>
		requires requires { std::hash<T>{}; std::hash<dpl::Terminal<S>>{}; }
	class hash<dpl::Token<T, S>> {
	public:
		//credit to boost::hash_combine
		std::size_t operator()(dpl::Token<T, S> const& t) const noexcept {
			size_t intermediate = std::hash<dpl::Terminal<S>>{}(static_cast<dpl::Terminal<S>>(t));
			intermediate ^= std::hash<typename dpl::Token<T, S>::value_type>{}(t.value)
				+ 0x9e3779b9 + (intermediate << 6) + (intermediate >> 2);
			return intermediate;
		}
	};

	template<typename T>
		requires requires { std::hash<T>{}; }
	class hash<dpl::Terminal<T>> {
	public:
		std::size_t operator()(dpl::Terminal<T> const& t) const noexcept {
			return std::hash<size_t>{}(t.name) ^ std::hash<bool>{}(t.type);
		}
	};
}