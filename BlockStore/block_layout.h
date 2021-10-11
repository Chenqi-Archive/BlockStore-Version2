#pragma once

#include "core.h"


BEGIN_NAMESPACE(BlockStore)


constexpr size_t block_index_invalid = -1;

template<class T>
constexpr bool is_block_type_valid = !std::is_pointer_v<T> && !std::is_reference_v<T> && std::is_trivially_constructible_v<T>;


struct memory_copyable_t {};
constexpr memory_copyable_t memory_copyable = {};


template<class T>
constexpr auto layout() {}

template<>
constexpr auto layout<std::enable_if_t<std::is_arithmetic_v<T>, T>>() { return memory_copyable; }


template<class... Ts, class T>
auto declare(Ts T::*... member_list) { return std::make_tuple(member_list...); }

template<class... Ts, class T>
auto get_member_type_tuple(std::tuple<Ts T::*...>) -> std::tuple<Ts...> { return {}; }

template<class... Ts>
constexpr bool is_memory_copyable = std::is_same_v<memory_copyable_t, decltype(layout<Ts>())> && ... && true;

template<class... Ts>
constexpr bool is_memory_copyable<std::tuple<Ts...>> = is_block_memory_copyable<Ts...>;

template<class T>
constexpr bool is_memory_copyable_indirect = is_block_memory_copyable<decltype(get_member_type_tuple(layout<T>()))>;

int f() {
	is_memory_copyable<int>;
}


END_NAMESPACE(BlockStore)