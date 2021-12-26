#pragma once

#include <type_traits>
#include <span>
#include <array>
#include <optional>
#include <any>
#include <unordered_set>

namespace dpl {

	template<typename AtomT>
	struct any_impl;



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

		constexpr match(span_type str_) : str({ str_ }) { }

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


	template <dpl::regex InnerT>
	struct between {

		using atom_type = InnerT::atom_type;

		constexpr between(size_t L, size_t M, const InnerT& inner_) : inner(inner_), Least(L), Most(M) {}

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter) const -> std::optional<decltype(iter)> {
			auto result = iter;

			for (int i = 0; i < Most; i++) {
				auto next_result = inner(result);

				if (next_result) result = *next_result;
				else {
					if (i >= Least) return result;
					else return std::nullopt;
				}
			}

			return result;
		}

	protected:
		InnerT inner;
		size_t Least, Most;
	};


	template<dpl::regex InnerT>
	struct at_least : public between<InnerT> {
		using atom_type = InnerT::atom_type;
		constexpr at_least(size_t L, const InnerT& inner_) : between<InnerT>(L, std::numeric_limits<size_t>::max(), inner_) {}
	private:
		using between<InnerT>::between;
	};

	template<dpl::regex InnerT>
	struct at_most : public between<InnerT> {
		using atom_type = InnerT::atom_type;
		constexpr at_most(size_t M, const InnerT& inner_) : between<InnerT>(0, M, inner_) {}
	private:
		using between<InnerT>::between;
	};

	template<dpl::regex InnerT>
	struct exactly : public between<InnerT> {
		using atom_type = InnerT::atom_type;
		constexpr exactly(size_t N, const InnerT& inner_) : between<InnerT>(N, N, inner_) {}
	private:
		using between<InnerT>::between;
	};

	template<dpl::regex InnerT>
	struct some : public between<InnerT> {
		using atom_type = InnerT::atom_type;
		constexpr some(const InnerT& inner_) : between<InnerT>(1, std::numeric_limits<size_t>::max(), inner_) {}
	private:
		using between<InnerT>::between;
	};

	template<dpl::regex InnerT>
	struct kleene : public between<InnerT> {
		using atom_type = InnerT::atom_type;
		constexpr kleene(const InnerT& inner_) : between<InnerT>(0, std::numeric_limits<size_t>::max(), inner_) {}
	private:
		using between<InnerT>::between;
	};


	


	template<typename AtomT = char>
	struct any_impl {
		using atom_type = AtomT;

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter) const -> std::optional<decltype(iter)> {
			return iter + 1;
		}
	};

	template<typename AtomT>
	constexpr any_impl<AtomT> any{};

	static_assert(dpl::regex<dpl::any_impl<>>);


	template<typename AtomT = char>
	struct any_of {
		using atom_type = match<AtomT>::atom_type;
		using span_type = match<AtomT>::span_type;

		constexpr any_of(span_type str_) : str(str_.begin(), str_.end()) { }

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter) const -> std::optional<decltype(iter)> {
			auto result = std::find(str.begin(), str.end(), *iter);

			if (result != str.end()) return iter + 1;
			else return std::nullopt;
		}

	protected:
		span_type str;

	};

	template<typename AtomT = char>
		requires std::totally_ordered<AtomT>
	struct range {
		using atom_type = match<AtomT>::atom_type;

		constexpr range(atom_type from_, atom_type to_) : from(from_), to(to_) { }

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter) const -> std::optional<decltype(iter)> {
			if (from <= *iter && *iter <= to) return iter + 1;
			else return std::nullopt;
		}

	protected:
		atom_type from, to;

	};

	constexpr dpl::range lower{ 'a', 'z' };
	constexpr dpl::range upper{ 'A', 'Z' };
	constexpr dpl::range digit{ '0', '9' };

	constexpr auto hex_digit = dpl::alternatives{
		dpl::digit,
		dpl::range{'a', 'f'},
		dpl::range{'A', 'F'}
	};

	constexpr auto alpha = dpl::alternatives{ dpl::lower, dpl::upper };
	constexpr auto alphanum = dpl::alternatives{ dpl::alpha, dpl::digit };
	constexpr dpl::any_of whiespace{ " \t\n\0" };

}