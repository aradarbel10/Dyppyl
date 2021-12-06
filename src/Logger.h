#pragma once

#include <windows.h>

#include <iostream>
#include <variant>
#include <vector>
#include <chrono>
#include <cmath>

#include "magic_enum/magic_enum.hpp"

namespace dpl {
	namespace log {

		struct Token;

		template<typename T>
		struct streamer {
			const T& val;
		};

		template<typename... Ts>
		inline std::ostream& operator<<(std::ostream& os, streamer<std::variant<Ts...>> vnt) {
			std::visit([&os](auto&& v) { os << streamer{ v }; }, vnt.val);
			return os;
		}

		template<typename T1, typename T2>
		inline std::ostream& operator<<(std::ostream& os, streamer<std::pair<T1, T2>> pr) {
			os << streamer{ pr.val.first } << ", " << streamer{ pr.val.second };
			return os;
		}

		template<typename T>
		inline std::ostream& operator<<(std::ostream& os, streamer<std::optional<T>> strm) {
			os << streamer{ strm.val.value_or("null") };
			return os;
		}

		template<typename T> requires std::is_enum_v<T>
		inline std::ostream& operator<<(std::ostream& os, streamer<T> strm) {
			os << magic_enum::enum_name(strm.val);
			return os;
		}

		template<typename T>
		inline std::ostream& operator<<(std::ostream& os, streamer<T> strm) {
			os << strm.val;
			return os;
		}

		inline std::ostream& operator<<(std::ostream& os, streamer<std::monostate> strm) {
			return os;
		}

		// #TASK : support colored printing on non-windows systems
		inline void coloredStream(std::ostream& os, int color, std::string_view str) {
			static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

			SetConsoleTextAttribute(hConsole, color);
			os << str;
			SetConsoleTextAttribute(hConsole, 0x07);
		}

		#ifdef DPL_LOG

		struct FileSize {
			std::uintmax_t size{};
		private: friend
			std::ostream& operator<<(std::ostream& os, FileSize hr) {
			int i{};
			double mantissa = static_cast<double>(hr.size);
			for (; mantissa >= 1024.; mantissa /= 1024., ++i) {}
			mantissa = std::ceil(mantissa * 10.) / 10.;
			os << mantissa << "BKMGTPE"[i];
			return i == 0 ? os : os << "B (" << hr.size << ')';
		}
		};

		template<typename... Ts>
		struct info_log {
			constexpr info_log(const char* header_) : header(header_) { }

			template<class S> requires std::disjunction_v<std::is_convertible<S, Ts>...>
			constexpr void add(std::string_view label, S val) {
				entries.emplace_back(std::make_pair(label, val));
			}

			constexpr void clear() {
				entries.clear();
			}

			void print(std::ostream& os) {
				os << "\n\n" << header << ":\n";
				for (const auto& [label, val] : entries) {
					os << '\t' << label << "  ---  " << dpl::log::streamer{ val } << '\n';
				}
			}

		private:
			std::vector<std::pair<std::string_view, std::variant<Ts...>>> entries;
			std::string_view header;
		};

		using dur_type = std::chrono::duration<double>;
		static info_log<dur_type, unsigned long long, long double, std::string, FileSize> telemetry_info{ "DPL timing results" };

		#endif

	}
}

#ifdef DPL_LOG
#define DplLogPrintTelemetry() dpl::log::telemetry_info.print(std::cout);
#else //DPL_LOG
#define DplLogPrintTelemetry()
#endif //DPL_LOG