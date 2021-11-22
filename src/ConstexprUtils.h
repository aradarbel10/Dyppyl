#pragma once

#include <array>
#include <initializer_list>
#include <algorithm>

namespace dpl::cc {

	template <typename T, size_t maxN>
	class static_vector {
	private:

		size_t sizeN = 0;
		std::array<T, maxN> arr;

	public:

		using value_type = T;
		using iterator_type = std::array<T, maxN>::iterator;

		constexpr static_vector() { }

		constexpr static_vector(std::initializer_list<T> l) {
			if (l.size() > maxN) throw std::out_of_range("initializer list cannot exceed max size!");
			std::copy(l.begin(), l.end(), arr.begin());
			sizeN = l.size();
		}

		[[nodiscard]] constexpr size_t size() const { return sizeN; }
		[[nodiscard]] constexpr bool empty() const { return sizeN == 0; }
		[[nodiscard]] constexpr T& operator[](size_t index) {
			if (sizeN >= index) throw std::out_of_range;
			return arr[index];
		}
		[[nodiscard]] constexpr auto begin() { return arr.begin(); }
		[[nodiscard]] constexpr auto end() { return std::next(arr.begin(), sizeN); }
		[[nodiscard]] constexpr const auto cbegin() const { return arr.cbegin(); }
		[[nodiscard]] constexpr const auto cend() const { return std::next(arr.cbegin(), sizeN); }
		[[nodiscard]] constexpr auto rbegin() { return std::next(arr.begin(), sizeN - 1); }
		[[nodiscard]] constexpr auto rend() { return std::next(arr.begin()); }
		constexpr void push_back(T&& t) {
			if (sizeN >= maxN) throw std::out_of_range("can't push_back into a vector with max size!");
			arr[sizeN++] = std::forward(t);
		}

		constexpr void insert(iterator_type dest) {
			if (sizeN >= maxN) throw std::out_of_range("can't insert into a vector with max size!");
			if (dest >= end()) throw std::out_of_range("can't insert past a vector's end!");

			sizeN++;
			std::copy(std::next(end()), dest, end());
		}
		constexpr void insert(size_t dest) {
			insert(begin() + dest);
		}

	};

	template <typename KeyT, typename ValT>
	class map : private std::vector<std::pair<KeyT, ValT>> {

		using std::vector<std::pair<KeyT, ValT>>::push_back;
		using std::vector<std::pair<KeyT, ValT>>::operator[];

	public:

		constexpr map(std::initializer_list<std::pair<KeyT, ValT>> il) : std::vector<std::pair<KeyT, ValT>>(il) { }
		constexpr void insert(const KeyT& key, const ValT& val) {
			if (!contains(key)) push_back({ key, val });
		}
		[[nodiscard]] constexpr ValT& operator[](const KeyT& key) {
			auto iter = std::find_if(begin(), end(), [&key](const auto& pair) { return pair.first == key; });
			if (iter == end()) {
				std::pair<KeyT, ValT> pair;
				pair.first = key;
				push_back(pair);
				return std::prev(end())->second;
			} else {
				return iter->second;
			}
		}
		[[nodiscard]] constexpr const ValT& at(const KeyT& key) const {
			return std::find_if(begin(), end(), [&key](const auto& pair) { return pair.first == key; })->second;
		}
		[[nodiscard]] constexpr const ValT& operator[](const KeyT& key) const {
			return at(key);
		}

		[[nodiscard]] constexpr bool contains(const KeyT& key) const {
			return std::any_of(begin(), end(), [&key](const auto& pair) { return pair.first == key; });
		}

		constexpr bool operator==(const map<KeyT, ValT>& rhs) {
			return std::is_permutation(begin(), end(), rhs.begin(), rhs.end());
		}

		using std::vector<std::pair<KeyT, ValT>>::vector;
		using std::vector<std::pair<KeyT, ValT>>::size;
		using std::vector<std::pair<KeyT, ValT>>::empty;
		using std::vector<std::pair<KeyT, ValT>>::begin;
		using std::vector<std::pair<KeyT, ValT>>::end;
		using std::vector<std::pair<KeyT, ValT>>::reserve;
		using std::vector<std::pair<KeyT, ValT>>::clear;

	};


	template <typename ValT>
	class set : private std::vector<ValT> {
	public:

		constexpr set(std::initializer_list<ValT> il) {
			for (auto i = il.begin(); i != il.end(); i++) {
				insert(*i);
			}
		}

		using std::vector<ValT>::vector;
		using std::vector<ValT>::size;
		using std::vector<ValT>::empty;
		using std::vector<ValT>::begin;
		using std::vector<ValT>::end;
		using std::vector<ValT>::cbegin;
		using std::vector<ValT>::cend;
		using std::vector<ValT>::rbegin;
		using std::vector<ValT>::rend;
		using std::vector<ValT>::reserve;

		constexpr void insert(const ValT& val) {
			if (!contains(val)) {
				this->push_back(val);
			};
		}

		constexpr bool contains(const ValT& val) const {
			for (int i = 0; i < size(); i++) {
				if (this->at(i) == val) return true;
			}
			return false;
		}

		constexpr bool erase(const ValT& val) {
			size_t size_before = size();
			this->std::vector<ValT>::erase(std::remove(begin(), end(), val));
			return size() != size_before;
		}

		constexpr auto find(const ValT& val) {
			return std::find(begin(), end(), val);
		}

		constexpr friend bool operator==(const set<ValT>& lhs, const set<ValT>& rhs) {
			return std::is_permutation(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
		}

	};


}