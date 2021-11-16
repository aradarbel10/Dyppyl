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
	class map {

		std::vector<std::pair<KeyT, ValT>> underlying;

		constexpr auto index_impl(const KeyT& key) {
			return std::find_if(begin(), end(), [&key](const auto& pair) { return pair.first == key; });
		}

		constexpr auto index_impl(const KeyT& key) const {
			return std::find_if(cbegin(), cend(), [&key](const auto& pair) { return pair.first == key; });
		}

		constexpr auto index_or_create_impl(const KeyT& key) {
			auto iter = index_impl(key);
			if (iter != end()) return iter;
			else {
				std::pair<KeyT, ValT> pair;
				pair.first = key;
				underlying.emplace_back(std::move(pair));

				return std::prev(end());
			}
		}

	public:

		constexpr map(std::initializer_list<std::pair<KeyT, ValT>> il) : underlying(il) { }
		constexpr map() = default;

		[[nodiscard]] constexpr size_t size() { return underlying.size(); }
		[[nodiscard]] constexpr bool empty() { return underlying.empty(); }

		[[nodiscard]] constexpr auto begin() const { return underlying.begin(); }
		[[nodiscard]] constexpr auto end() const { return underlying.end(); }
		[[nodiscard]] constexpr auto begin() { return underlying.begin(); }
		[[nodiscard]] constexpr auto end() { return underlying.end(); }
		[[nodiscard]] constexpr auto cbegin() const { return underlying.cbegin(); }
		[[nodiscard]] constexpr auto cend() const { return underlying.cend(); }

		constexpr void reserve(size_t s) { return underlying.reserve(s); }
		constexpr void clear() { underlying.clear(); }

		[[nodiscard]] constexpr ValT& operator[](const KeyT& key) { return index_or_create_impl(key)->second; }
		[[nodiscard]] constexpr const ValT& at(const KeyT& key) const { return index_impl(key)->second; }

		[[nodiscard]] constexpr bool contains(const KeyT& key) const {
			return std::any_of(cbegin(), cend(), [&key](const auto& pair) { return pair.first == key; });
		}

		constexpr friend bool operator==(const map<KeyT, ValT>& lhs, const map<KeyT, ValT>& rhs) {
			return std::is_permutation(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
		}

	};


	template <typename ValT>
	class set : private std::vector<ValT> {
	public:

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

		constexpr bool insert(const ValT& val) {
			if (!contains(val)) {
				this->push_back(val);
				return true;
			} else return false;
		}

		constexpr bool contains(const ValT& val) const {
			return std::find(begin(), end(), val) != end();
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