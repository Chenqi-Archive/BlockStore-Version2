#pragma once

#include "core.h"


BEGIN_NAMESPACE(BlockStore)


constexpr size_t block_alignment = sizeof(size_t);

static_assert(block_alignment == 4 || block_alignment == 8);

constexpr size_t next_offset(size_t offset, size_t size) {
	return ((offset + size) + (block_alignment - 1)) & ~(block_alignment - 1);
}


struct memcpy_initializable_t {};
constexpr memcpy_initializable_t memcpy_initializable = {};


template<class T>
struct layout_type {};

template<class T, class... Ts>
constexpr auto declare(Ts T::*... member_list) { return std::make_tuple(member_list...); }

template<class T, class... Ts>
constexpr auto member_type_tuple(std::tuple<Ts T::*...>) -> std::tuple<Ts...> { return {}; }


template<class T, class = void>
constexpr bool is_memcpy_initializable = false;

template<class T>
constexpr bool is_memcpy_initializable<T, std::enable_if_t<std::is_same_v<memcpy_initializable_t, decltype(layout(layout_type<T>()))>>> = true;


template<class T, class = std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr auto layout(layout_type<T>) { return memcpy_initializable; }


constexpr size_t block_size_dynamic = -1;

template<class T, class = void>
struct block_size_t {
	static constexpr size_t value = block_size_dynamic;
};

template<class T>
struct block_size_t<T, std::enable_if_t<is_memcpy_initializable<T>>> {
	static constexpr size_t value = sizeof(T);
};

template<class T, class... Ts>
struct block_size_t<std::tuple<T, Ts...>> {
	static constexpr size_t calculate_size() {
		size_t size_first = block_size_t<T>::value;
		if (size_first == block_size_dynamic) { return block_size_dynamic; }
		size_t size_rest = block_size_t<std::tuple<Ts...>>::value;
		if (size_rest == block_size_dynamic) { return block_size_dynamic; }
		return size_first + size_rest;
	}
	static constexpr size_t value = calculate_size();
};

template<class T>
struct block_size_t<std::tuple<T>> : block_size_t<T> {};

template<class T>
struct block_size_t<T, decltype(member_type_tuple(layout(layout_type<T>())), void())> : block_size_t<decltype(member_type_tuple(layout(layout_type<T>())))> {};

template<class T>
constexpr size_t block_size = block_size_t<T>::value;

template<class T>
constexpr bool is_block_size_fixed = !is_memcpy_initializable<T> && block_size<T> != block_size_dynamic;

template<class T>
constexpr bool is_block_size_dynamic = !is_memcpy_initializable<T> && block_size<T> == block_size_dynamic;


END_NAMESPACE(BlockStore)