#pragma once

#include "block_layout.h"

#include <string>
#include <vector>
#include <array>
#include <list>
#include <variant>


BEGIN_NAMESPACE(BlockStore)


template<class T, size_t length>
struct block_size_t<std::array<T, length>> {
	static constexpr size_t calculate_size() {
		size_t size = block_size_t<T>::value;
		if (size == block_size_dynamic) { return block_size_dynamic; }
		return size * length;
	}
	static constexpr size_t value = calculate_size();
};


template<class T, class... Ts>
struct block_size_t<std::variant<T, Ts...>> {
	static constexpr size_t calculate_size() {
		size_t size_first = block_size_t<T>::value;
		if (size_first == block_size_dynamic) { return block_size_dynamic; }
		size_t size_rest = block_size_t<std::variant<Ts...>>::value;
		if (size_rest == block_size_dynamic) { return block_size_dynamic; }
		if (size_first + sizeof(size_t) != size_rest) { return block_size_dynamic; }
		return size_rest;
	}
	static constexpr size_t value = calculate_size();
};

template<class T>
struct block_size_t<std::variant<T>> {
	static constexpr size_t calculate_size() {
		size_t size = block_size_t<T>::value;
		if (size == block_size_dynamic) { return block_size_dynamic; }
		return size + sizeof(size_t);
	}
	static constexpr size_t value = calculate_size();
};


END_NAMESPACE(BlockStore)