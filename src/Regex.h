#pragma once

#include <type_traits>
#include <span>
#include <array>

namespace dpl {
	template<typename AtomT = char>
	struct regex {
		using raw_atom_type = AtomT;
		using atom_type = std::remove_cv_t<AtomT>;

		constexpr auto& operator<<(const atom_type& atom) {
			if (alive) {
				just_accepted = on_accept_state();

				if (feed(atom)) age++;
				else kill();
			}

			return *this;
		}

		constexpr size_t length() const { return age; }
		constexpr bool is_alive() const { return alive; }
		constexpr bool accepted() const { return just_accepted; }
		constexpr void reset() { age = 0; alive = true; init(); }

		constexpr virtual bool on_accept_state() const = 0; // returns whether the NFA is in an accepting state

	protected:
		constexpr virtual bool feed(const atom_type& atom) = 0; // steps the NFA one state forward or die
		constexpr virtual void init() = 0; // reset the NFA to the initial state

		constexpr void kill() {
			alive = false;
		}

	private:
		bool alive = true;
		bool just_accepted = false;
		size_t age = 0;

	private:
		template<typename>
		friend struct match;

		template<typename AtomT_, typename ...AltsTs_>
			requires (std::derived_from<AltsTs_, dpl::regex<AtomT_>> && ...)
		friend struct alternatives;

	};

	template<typename AtomT = char>
	struct match : public regex<AtomT> {
		
		using atom_type = regex<AtomT>::atom_type;
		using span_type = std::conditional_t<std::is_same_v<atom_type, char>, std::string_view, std::span<const atom_type>>;
		// #TASK : map char -> string_view, wchar -> wstring_view, etc...

		constexpr match(span_type str_) : str(str_) {
			states.resize(str.size() + 1);
		}

		constexpr bool on_accept_state() const override { return states.back(); }

	protected:
		constexpr bool feed(const atom_type& atom) override {
			std::vector<bool> next;
			next.resize(states.size());

			bool all_dead = true;

			next.back() = false;
			for (int i = 0; i < str.size(); i++) {
				next[i + 1] = (states[i] && str[i] == atom);
				if (next[i + 1]) all_dead = false;
			}

			std::swap(states, next);
			return !all_dead;
		}
		
		constexpr void init() override { states.front() = true; }

		std::vector<bool> states;
		span_type str;

	};

	template<typename AtomT, typename ...AltsTs>
		requires (std::derived_from<AltsTs, dpl::regex<AtomT>> && ...)
	struct alternatives : public regex<AtomT> {

		using atom_type = regex<AtomT>::atom_type;

		constexpr alternatives(const AltsTs&... alts_) : alts({ alts_... }) { }

		constexpr bool on_accept_state() const override {
			// if any of the alternatives accepted
			return std::apply([&](auto&&... alt) {
				return (alt.on_accept_state() || ...);
			}, alts);
		}

	protected:
		constexpr bool feed(const atom_type& atom) override {
			// feed to each alt. is alive if any of the alts are still alive (max munch)
			return std::apply([&](auto&&... alt) {
				((alt << atom), ...);
				return (alt.is_alive() || ...);
			}, alts);
		}

		constexpr void init() override {
			std::apply([](auto&&... alt) {
				// must use inner lambda to unpack member application
				(std::invoke([](auto&& alt_) { alt_.reset(); }, alt), ...);
			}, alts);
		}

		std::tuple<AltsTs...> alts;

	};

	template<typename ...AltsTs>
	alternatives(const AltsTs&... alts_) -> alternatives<typename std::tuple_element_t<0, std::tuple<AltsTs...>>::raw_atom_type, AltsTs...>;


	template<typename AtomT, typename ...SubsTs>
		requires (std::derived_from<SubsTs, dpl::regex<AtomT>> && ...)
	struct sequence : public regex<AtomT> {

		using atom_type = regex<AtomT>::atom_type;

		constexpr sequence(const SubsTs&... subs_) : subs({ subs_... }) {
			refs = std::apply([](auto&&... refs_) {
				return std::array{ static_cast<dpl::regex<AtomT>*>(&refs_)... };
			}, subs);
		}

		constexpr bool on_accept_state() const override {
			// if the very last sub accepted
			return refs.back()->on_accept_state();
		}

	protected:
		constexpr bool feed(const atom_type& atom) override {
			bool any_alive = false;

			for (int i = 0; i < sizeof...(SubsTs); i++) {
				(*refs[i]) << atom;

				if (i < sizeof...(SubsTs) - 1 && refs[i]->accepted()) {
					refs[i + 1]->reset();
				}

				if (refs[i]->is_alive()) any_alive = true;
			}

			return any_alive;
		}

		constexpr void init() override {
			refs.front()->reset();
		}

		std::tuple<SubsTs...> subs;

		std::array<dpl::regex<AtomT>*, sizeof...(SubsTs)> refs; // reference to the tuple's elements for dynamic indexed access

	};

	template<typename ...SubsTs>
	sequence(const SubsTs&... alts_) -> sequence<typename std::tuple_element_t<0, std::tuple<SubsTs...>>::raw_atom_type, SubsTs...>;
}