#pragma once


#include <iostream>
#include <variant>
#include <vector>
#include <chrono>

#include "magic_enum/magic_enum.hpp"

namespace dpl {
	namespace log {

		template<typename KwdT, typename SymT> requires std::is_enum_v<KwdT>&& std::is_enum_v<SymT> struct Token;

		template<typename T>
		struct streamer {
			const T& val;
		};


		template<typename KwdT, typename SymT>
		std::ostream& operator<<(std::ostream& os, const streamer<Token<KwdT, SymT>>& t) {
			os << t.val;
			return os;
		}

		template<typename... Ts>
		inline std::ostream& operator<<(std::ostream& os, streamer<std::variant<Ts...>> vnt) {
			std::visit([&os](auto&& v) { os << streamer{ v }; }, vnt.val);
			return os;
		}

		template<typename T1, typename T2>
		inline std::ostream& operator<<(std::ostream& os, streamer<std::pair<T1, T2>> pr) {
			os << pr.val.first << ", " << streamer{ pr.val.second };
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

		#ifdef DPL_LOG

		struct FileSize {
			std::uintmax_t size{};
		private: friend
			std::ostream& operator<<(std::ostream& os, FileSize hr) {
			int i{};
			double mantissa = hr.size;
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
		static info_log<dur_type, unsigned long long, long double, std::string, FileSize> telemetry_info{"DPL timing results"};

		#endif

	}
}

#ifdef DPL_LOG
#define DplLogPrintTelemetry() dpl::log::telemetry_info.print(std::cout);
#else //DPL_LOG
#define DplLogPrintTelemetry()
#endif //DPL_LOG