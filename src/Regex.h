#pragma once

#include <type_traits>
#include <span>
#include <array>
#include <optional>

namespace dpl {
	template<typename IterT, typename AtomT>
	concept initer_of_type = std::input_iterator<IterT> && std::is_same_v<typename IterT::value_type, AtomT>;

	template<class RegexT>
	concept regex =
		requires { typename RegexT::atom_type; }
		&& requires (RegexT r, typename std::span<typename RegexT::atom_type>::iterator iter) {
			{ r(iter) } -> std::same_as<std::optional<decltype(iter)>>;
		};

	template<class RegexT1, class RegexT2>
	concept compatible_regex_pair =
		regex<RegexT1>
		&& regex<RegexT2>
		&& std::is_same_v<typename RegexT1::atom_type, typename RegexT2::atom_type>;

	template<class RegexT, class ...RegexTs>
	concept compatible_regex = (compatible_regex_pair<RegexT, RegexTs> && ...);

	template<typename AtomT = char>
	struct match {
		using atom_type = AtomT;
		using span_type = std::conditional_t<std::is_same_v<atom_type, char>, std::string_view, std::span<const atom_type>>;
		// #TASK : map char -> string_view, wchar -> wstring_view, etc...

		constexpr match(span_type str_) : str(str_) { }

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter) const -> std::optional<decltype(iter)> {
			for (int i = 0; i < str.size(); i++) {
				if (*iter != str[i]) return std::nullopt;
				else iter++;
			}
			return std::optional{ iter };
		}

	protected:
		
		span_type str;

	};

	static_assert(dpl::regex<dpl::match<>>);

	template<class ...AltsTs>
		requires dpl::compatible_regex<AltsTs...>
	struct alternatives {

		using atom_type = std::tuple_element_t<0, std::tuple<AltsTs...>>::atom_type;

		constexpr alternatives(const AltsTs&... alts_) : alts({ alts_... }) { }

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter) const -> std::optional<decltype(iter)> {
			return std::apply([&](auto&&... alts_) {
				return impl(iter, alts_...);
			}, alts);
		}

		

	protected:

		std::tuple<AltsTs...> alts;

		template<typename IterT>
		constexpr static auto impl(IterT iter, auto&& first, auto&&... rest) -> std::optional<decltype(iter)> {
			auto result = first(iter);

			if (result) return result;
			else {
				if constexpr (sizeof...(rest) > 0) return impl(iter, rest...);
				else return std::nullopt;
			}
		}

	};


	template<class ...SubsTs>
		requires dpl::compatible_regex<SubsTs...>
	struct sequence {

		using atom_type = std::tuple_element_t<0, std::tuple<SubsTs...>>::atom_type;

		constexpr sequence(const SubsTs&... subs_) : subs({ subs_... }) {}

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter) const -> std::optional<decltype(iter)> {
			return std::apply([&](auto&&... subs_) {
				return impl(iter, subs_...);
			}, subs);
		}

	protected:

		std::tuple<SubsTs...> subs;

		template<typename IterT>
		constexpr static auto impl(IterT iter, auto&& first, auto&&... rest) -> std::optional<decltype(iter)> {
			auto result = first(iter);

			if (result) {
				if constexpr (sizeof...(rest) > 0) return impl(*result, rest...);
				else return result;
			} else return std::nullopt;
		}
	};


	template <dpl::regex SubT>
	struct maybe {

		using atom_type = SubT::atom_type;

		constexpr maybe(const SubT& sub_) : sub(sub_) {}

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter) const -> std::optional<decltype(iter)> {
			auto result = sub(iter);

			if (result) return result;
			else return iter;
		}

	protected:
		SubT sub;

	};
}