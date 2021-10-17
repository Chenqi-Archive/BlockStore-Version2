#pragma once

#include "block_manager.h"


BEGIN_NAMESPACE(BlockStore)


template<class T>
struct layout_type {};

template<class T>
constexpr auto layout(layout_type<T>) {}

template<class T, class... Ts>
constexpr auto declare(Ts T::*... member_list) { return std::make_tuple(member_list...); }


template<class T, class... Ts>
constexpr auto member_type_tuple(std::tuple<Ts T::*...>) -> std::tuple<Ts...> { return {}; }

template<class T, class = void>
constexpr bool has_custom_layout = false;

template<class T>
constexpr bool has_custom_layout<T, decltype(member_type_tuple(layout(layout_type<T>())), void())> = true;

template<class T>
constexpr bool has_trivial_layout = std::is_trivial_v<T> && !has_custom_layout<T>;


template<class T>
constexpr void align_offset(size_t& offset) {
	constexpr size_t alignment = sizeof(T) <= 8 ? sizeof(T) : 8;
	static_assert((alignment & (alignment - 1)) == 0);  // 1, 2, 4, 8
	offset = (offset + (alignment - 1)) & ~(alignment - 1);
}


struct BlockLoadContext {
private:
	BlockManager& manager;
	const ref_ptr<const void> data;
	const size_t length;
	size_t offset;
public:
	BlockLoadContext(const void* data, size_t length) : data(data), length(length), offset(0) {}
private:
	void CheckNextOffset(size_t offset) { if (offset > length) { throw std::runtime_error("block size mismatch"); } }
public:
	template<class T>
	void read(T& object) {
		align_offset<T>(offset); size_t next = offset + sizeof(T); CheckNextOffset(next);
		memcpy(&object, data + offset, sizeof(T)); offset = next;
	}
	template<class T>
	void read(T object[], size_t count) {
		align_offset<T>(offset); size_t next = offset + sizeof(T) * count; CheckNextOffset(next);
		memcpy(object, data + offset, sizeof(T) * count); offset = next;
	}
public:
	template<class T>
	void read(BlockRef<T> block_ref) {
		read(block_ref.index); block_ref.manager = manager;
	}
};

struct BlockSaveContext {
private:
	const ref_ptr<void> data;
	const size_t length;
	size_t offset;
public:
	BlockSaveContext(void* data, size_t length) : data(data), length(length), offset(0) {}
private:
	void CheckNextOffset(size_t offset) { if (offset > length) { throw std::runtime_error("block size mismatch"); } }
public:
	template<class T>
	void write(const T& object) {
		align_offset<T>(offset); size_t next = offset + sizeof(T); CheckNextOffset(next);
		memcpy(data + offset, &object, sizeof(T)); offset = next;
	}
	template<class T>
	void write(const T object[], size_t count) {
		align_offset<T>(offset); size_t next = offset + sizeof(T) * count; CheckNextOffset(next);
		memcpy(data + offset, object, sizeof(T) * count); offset = next;
	}
public:
	template<class T>
	void write(BlockRef<T> block_ref) {
		write(block_ref.index);
		manager->WriteBlock(block_ref);
	}
};


template<class T, class = void>
struct layout_traits {
	static_assert(false, "block layout undefined");
	static size_t CalculateSize(const T& object) { return 0; }
	static void Load(BlockLoadContext& context, T& object) {}
	static void Save(BlockSaveContext& context, const T& object) {}
};

template<class T> size_t CalculateSize(const T& object) { return layout_traits<T>::CalculateSize(object); }
template<class T> void Load(BlockLoadContext& context, T& object) { layout_traits<T>::Load(context, object); }
template<class T> void Save(BlockLoadContext& context, const T& object) { layout_traits<T>::Save(context, object); }


template<class T>
struct layout_traits<T, std::enable_if_t<has_trivial_layout<T>>> {
	static size_t CalculateSize(const T& object) { return sizeof(T); }
	static void Load(BlockLoadContext& context, T& object) { context.read(object); }
	static void Save(BlockSaveContext& context, const T& object) { context.write(object); }
};


template<class T>
struct layout_traits<T, std::enable_if_t<has_custom_layout<T>>> {
	static size_t CalculateSize(const T& object) {
		return std::apply([&](auto... member) { return (BlockStore::CalculateSize(object.*member) + ...); }, layout(layout_type<T>()));
	}
	static void Load(BlockLoadContext& context, T& object) {
		std::apply([&](auto... member) { (BlockStore::Load(context, object.*member), ...); }, layout(layout_type<T>()));
	}
	static void Save(BlockSaveContext& context, const T& object) {
		std::apply([&](auto... member) { (BlockStore::Save(context, object.*member), ...); }, layout(layout_type<T>()));
	}
};


template<class T1, class T2>
struct layout_traits<std::pair<T1, T2>> {
	static constexpr size_t CalculateSize(const std::pair<T1, T2>& object) {
		return BlockStore::CalculateSize(object.first) + BlockStore::CalculateSize(object.second);
	}
	static void Load(BlockLoadContext& context, std::pair<T1, T2>& object) {
		BlockStore::Load(context, object.first); BlockStore::Load(context, object.second);
	}
	static void Save(BlockSaveContext& context, const std::pair<T1, T2>& object) {
		BlockStore::Save(context, object.first); BlockStore::Save(context, object.second);
	}
};


template<class... Ts>
struct layout_traits<std::tuple<Ts...>> {
	static constexpr size_t CalculateSize(const std::tuple<Ts...>& object) {
		return std::apply([&](auto&... member) { return (BlockStore::CalculateSize(member) + ...); }, object);
	}
	static void Load(BlockLoadContext& context, std::tuple<Ts...>& object) {
		return std::apply([&](auto&... member) { (BlockStore::Load(context, member), ...); }, object);
	}
	static void Save(BlockSaveContext& context, const std::tuple<Ts...>& object) {
		return std::apply([&](auto&... member) { (BlockStore::Save(context, member), ...); }, object);
	}
};


END_NAMESPACE(BlockStore)