#pragma once

#undef max
#undef min

#include <iostream>
#include <variant>
#include <vector>
#include <chrono>
#include <cmath>
#include <sstream>
#include <optional>

namespace dpl {
	using file_pos_t = size_t;

	template<typename, typename> struct Token;

	template<typename T>
	struct streamer {
		const T& val;
		const int color = 0x07;
	};

	struct color_cout {
		int color = 0x07;

		friend const color_cout& operator<<(const color_cout& os, const auto& other) {
			//SetConsoleTextAttribute(hConsole, os.color);
			std::cout << other;
			//SetConsoleTextAttribute(hConsole, 0x07);

			return os;
		}

	private:
		//inline static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	};

	// #TASK : support colored printing on non-windows systems
	template <typename T>
		requires requires (std::ostream& os, const T& t) { os << t; }
	inline void colored_stream(std::ostream& os, int color, const T& str) {
		//static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		//SetConsoleTextAttribute(hConsole, color);
		os << str;
		//SetConsoleTextAttribute(hConsole, 0x07);
	}

	template<typename T>
		requires requires (std::ostream& os, const T& t) { os << t; }
	inline std::ostream& operator<<(std::ostream& os, streamer<T> strm) {
		if (strm.color != 0x07) colored_stream(os, strm.color, strm.val);
		else os << strm.val;

		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, streamer<std::string_view> str) {
		os << str.val;
		return os;
	}

	template<typename... Ts>
	inline std::ostream& operator<<(std::ostream& os, streamer<std::variant<Ts...>> vnt) {
		std::visit([&os, &vnt](auto&& v) { os << streamer{ v, vnt.color }; }, vnt.val);
		return os;
	}

	template<typename T1, typename T2>
	inline std::ostream& operator<<(std::ostream& os, streamer<std::pair<T1, T2>> pr) {
		os << streamer{ pr.val.first, pr.color } << streamer{ ", ", pr.color } << streamer{ pr.val.second, pr.color };
		return os;
	}

	template<typename T>
	inline std::ostream& operator<<(std::ostream& os, streamer<std::optional<T>> strm) {
		os << streamer{ strm.val.value_or("null"), strm.color };
		return os;
	}

	template<typename T, template <typename> class Container>
		requires requires (Container<T> cont) { cont.begin(); cont.end(); }
	inline std::ostream& operator<<(std::ostream& os, streamer<Container<T>> cont) {
		os << streamer{ *cont.val.begin() };
		for (auto iter = std::next(cont.val.begin()); iter != cont.val.end(); ++iter) {
			os << streamer{ ", ", cont.color } << streamer{ *iter, cont.color };
		}
		return os;
	}

	template<typename T, size_t N, template <typename, size_t> class Container>
		requires requires (Container<T, N> cont) { cont.begin(); cont.end(); }
	inline std::ostream& operator<<(std::ostream& os, streamer<Container<T, N>> cont) {
		if (cont.val.empty()) return os;

		os << streamer{ *cont.val.begin() };
		for (auto iter = std::next(cont.val.begin()); iter != cont.val.end(); ++iter) {
			os << streamer{ ", ", cont.color } << streamer{ *iter, cont.color };
		}
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, streamer<std::monostate> strm) {
		os << "null";
		return os;
	}

	std::string to_string(const auto& t) {
		std::stringstream ss;
		ss << dpl::streamer{ t };
		return ss.str();
	}

	struct FileSize {
		std::uintmax_t size{};
	private:
		friend std::ostream& operator<<(std::ostream& os, FileSize hr) {
			int i;
			double mantissa = static_cast<double>(hr.size);
			for (; mantissa >= 1024.; mantissa /= 1024., ++i) {}
			mantissa = std::ceil(mantissa * 10.) / 10.;
			os << mantissa << "BKMGTPE"[i];
			return i == 0 ? os : os << "B (" << hr.size << ')';
		}
	};
}