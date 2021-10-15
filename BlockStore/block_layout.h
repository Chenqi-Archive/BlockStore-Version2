#pragma once

#include "core.h"


BEGIN_NAMESPACE(BlockStore)


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


template<class T>
constexpr size_t align_offset(size_t offset) {
	constexpr size_t block_size = sizeof(T), alignment = block_size <= 8 ? block_size : 8;
	static_assert(alignment & (alignment - 1) == 0);  // 1, 2, 4, 8
	return (offset + (alignment - 1)) & ~(alignment - 1);
}


template<class T, class = void>
struct BlockTraits {
	using BlockType = T;
	static void Load(BlockLoadContext context, size_t& offset, T& object) {}
	static void Save(BlockSaveContext context, size_t& offset, const T& object) {}
	static size_t CalculateSize(const T& object) { return 0; }
};


END_NAMESPACE(BlockStore)