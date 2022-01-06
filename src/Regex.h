#pragma once

#include <type_traits>
#include <span>
#include <array>
#include <vector>
#include <optional>
#include <variant>
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
			{ r(iter, iter) } -> std::same_as<std::optional<decltype(iter)>>;
		};

	template<typename AtomT = char> struct regex_wrapper;

	template<class RegexT1, class RegexT2>
	concept compatible_regex_pair =
		regex<RegexT1>
		&& regex<RegexT2>
		&& std::is_same_v<typename RegexT1::atom_type, typename RegexT2::atom_type>;

	template<class RegexT, class ...RegexTs>
	concept compatible_regex = (compatible_regex_pair<RegexT, RegexTs> && ...);


	template<typename T>
	struct span_dict_ {
		using type = std::span<const T>;
	};

	template<>
	struct span_dict_<char> {
		using type = std::string_view;
	};

	template<typename T>
	using span_dict = span_dict_<T>::type;

	static_assert(std::is_same_v<span_dict<char>, std::string_view>);


	template<typename AtomT = char>
	struct regex_wrapper;



	template<typename AtomT = char>
	struct match {
		using atom_type = AtomT;
		using span_type = span_dict<atom_type>;

		constexpr match(span_type str_) : str({ str_ }) { }

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter, IterT end) const -> std::optional<decltype(iter)> {
			for (int i = 0; i < str.size(); i++) {
				if (iter == end || *iter != str[i]) return std::nullopt;
				else iter++;
			}
			return std::optional{ iter };
		}

	protected:
		span_type str;

	};

	static_assert(dpl::regex<dpl::match<>>);

	template<typename AtomT = char>
	struct alternatives {

		using atom_type = AtomT;

		template<dpl::regex ...AltsTs>
		alternatives(const AltsTs&... alts_) : alts({ dpl::regex_wrapper<atom_type>(alts_)... }) { }

		template<initer_of_type<atom_type> IterT>
		auto operator()(IterT iter, IterT end) const -> std::optional<decltype(iter)> {
			for (const auto& alt : alts) {
				auto result = alt(iter, end);
				if (result) return result;
			}
			return std::nullopt;
		}

	protected:
		std::vector<dpl::regex_wrapper<atom_type>> alts;
	};


	template<dpl::regex ...AltsTs>
	alternatives(const AltsTs&...) -> alternatives<typename std::tuple_element_t<0, std::tuple<AltsTs...>>::atom_type>;

	template<typename AtomT = char>
	struct sequence {

		using atom_type = AtomT;

		template<dpl::regex ...SubsTs>
		sequence(const SubsTs&... subs_) : subs({ subs_... }) { }

		template<initer_of_type<atom_type> IterT>
		auto operator()(IterT iter, IterT end) const -> std::optional<decltype(iter)> {
			auto head = iter;
			for (const auto& sub : subs) {
				auto result = sub(head, end);

				if (result) head = *result;
				else return std::nullopt;
			}
			return head;
		}

	protected:

		std::vector<dpl::regex_wrapper<atom_type>> subs;
	};

	template<typename T>
	struct ruleof3_ptr {

		ruleof3_ptr(const T& other)
			: inner(new T{ other }) {}

		ruleof3_ptr(const ruleof3_ptr<T>& other)
			: ruleof3_ptr(*other.inner) {}

		auto& operator=(const ruleof3_ptr<T>& other) {
			delete inner;
			inner = new T(*other.inner);
			return *this;
		}

		~ruleof3_ptr() { delete inner; }

	protected:
		T *inner;
	};


	template<typename AtomT>
	struct maybe : public ruleof3_ptr<dpl::regex_wrapper<AtomT>> {

		using atom_type = AtomT;

		constexpr maybe(const dpl::regex auto& sub_)
			: ruleof3_ptr<dpl::regex_wrapper<AtomT>>(sub_) {}

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter, IterT end) const -> std::optional<decltype(iter)> {
			auto result = (*this->inner)(iter, end);

			if (result) return result;
			else return iter;
		}
	};

	template<dpl::regex SubT>
	maybe(const SubT& sub_) -> maybe<typename SubT::atom_type>;

	template<typename AtomT = char>
	struct between : public ruleof3_ptr<dpl::regex_wrapper<AtomT>> {

		using atom_type = AtomT;

		constexpr between(size_t L, size_t M, const dpl::regex auto& inner_)
			: ruleof3_ptr<dpl::regex_wrapper<AtomT>>(inner_), Least(L), Most(M) {}

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter, IterT end) const -> std::optional<decltype(iter)> {
			auto result = iter;

			for (int i = 0; i < Most; i++) {
				auto next_result = (*this->inner)(result, end);

				if (next_result) result = *next_result;
				else {
					if (i >= Least) return result;
					else return std::nullopt;
				}
			}

			return result;
		}

	protected:
		size_t Least, Most;
	};


	template<typename AtomT = char>
	struct at_least : public between<AtomT> {
		using atom_type = between<AtomT>::atom_type;
		constexpr at_least(size_t L, const dpl::regex auto& inner_) : between<atom_type>(L, std::numeric_limits<size_t>::max(), inner_) {}
		constexpr ~at_least() {}
	private:
		using between<atom_type>::between;
	};

	template<typename AtomT = char>
	struct at_most : public between<AtomT> {
		using atom_type = between<AtomT>::atom_type;
		constexpr at_most(size_t M, const dpl::regex auto& inner_) : between<atom_type>(0, M, inner_) {}
		constexpr ~at_most() {}
	private:
		using between<AtomT>::between;
	};

	template<typename AtomT = char>
	struct exactly : public between<AtomT> {
		using atom_type = between<AtomT>::atom_type;
		constexpr exactly(size_t N, const dpl::regex auto& inner_) : between<atom_type>(N, N, inner_) {}
		constexpr ~exactly() {}
	private:
		using between<AtomT>::between;
	};

	template<typename AtomT = char>
	struct some : public between<AtomT> {
		using atom_type = between<AtomT>::atom_type;
		constexpr some(const dpl::regex auto& inner_) : between<atom_type>(1, std::numeric_limits<size_t>::max(), inner_) {}
		constexpr ~some() {}
	private:
		using between<AtomT>::between;
	};

	template<typename AtomT = char>
	struct kleene : public between<AtomT> {
		using atom_type = between<AtomT>::atom_type;
		constexpr kleene(const dpl::regex auto& inner_) : between<atom_type>(0, std::numeric_limits<size_t>::max(), inner_) {}
		constexpr ~kleene() {}
	private:
		using between<AtomT>::between;
	};


	


	template<typename AtomT = char>
	struct any_impl {
		using atom_type = AtomT;

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter, IterT end) const -> std::optional<decltype(iter)> {
			if (iter == end) return std::nullopt;
			else return iter + 1;
		}
	};

	template<typename AtomT>
	constexpr any_impl<AtomT> any{};

	static_assert(dpl::regex<dpl::any_impl<>>);


	template<typename AtomT = char>
	struct any_of {
		using atom_type = AtomT;
		using span_type = span_dict<atom_type>;

		constexpr any_of(span_type str_) : str(str_.begin(), str_.end()) { }

		template<initer_of_type<atom_type> IterT>
		constexpr auto operator()(IterT iter, IterT end) const -> std::optional<decltype(iter)> {
			if (iter == end) return std::nullopt;
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
		constexpr auto operator()(IterT iter, IterT end) const -> std::optional<decltype(iter)> {
			if (iter == end) return std::nullopt;
			else if (from <= *iter && *iter <= to) return iter + 1;
			else return std::nullopt;
		}

	protected:
		atom_type from, to;

	};

	static dpl::range lower{ 'a', 'z' };
	static dpl::range upper{ 'A', 'Z' };
	static dpl::range digit{ '0', '9' };

	static auto hex_digit = dpl::alternatives{
		dpl::digit,
		dpl::range{'a', 'f'},
		dpl::range{'A', 'F'}
	};

	static auto alpha = dpl::alternatives{ dpl::lower, dpl::upper };
	static auto alphanum = dpl::alternatives{ dpl::alpha, dpl::digit };
	static dpl::any_of whitespace{ " \t\n\0" };

	template<typename AtomT>
	struct regex_wrapper {
	private:
		using variant_type = std::variant<
			dpl::match<AtomT>,
			dpl::alternatives<AtomT>,
			dpl::sequence<AtomT>,
			dpl::maybe<AtomT>,
			dpl::some<AtomT>,
			dpl::kleene<AtomT>,
			dpl::exactly<AtomT>,
			dpl::at_least<AtomT>,
			dpl::at_most<AtomT>,
			dpl::between<AtomT>,
			dpl::any_impl<AtomT>,
			dpl::range<std::conditional_t<std::totally_ordered<AtomT>, AtomT, std::monostate>>,
			dpl::any_of<AtomT>
		>;
		variant_type r;

	public:
		using atom_type = AtomT;

		regex_wrapper(const dpl::regex auto& r_) : r(r_) {}

		auto& operator=(const regex_wrapper<atom_type>& other) {
			r = other.r;
			return (*this);
		}

		template<initer_of_type<atom_type> IterT>
		auto operator()(IterT iter, IterT end) const -> std::optional<decltype(iter)> {
			return std::visit([&](auto&& r_) { return r_(iter, end); }, r);
		}

	};

	template<typename T>
	constexpr T from_string(dpl::span_dict<char>) = delete;

	template<>
	constexpr int from_string(dpl::span_dict<char> str) {
		int result = 0;
		int i = 1;
		int factor;

		if (str[0] == '+') factor = 1;
		else if (str[0] == '-') factor = -1;
		else i = 0;

		for (; i < str.size(); ++i) {
			result *= 10;
			result += str[i] & 0b00001111;
		}

		return result;
	}

}