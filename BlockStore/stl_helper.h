#pragma once

#include "block_traits.h"

#include <string>
#include <vector>
#include <array>
#include <variant>


BEGIN_NAMESPACE(BlockStore)


template<class T>
struct layout_traits<std::basic_string<T>, std::enable_if_t<has_trivial_layout<T>>> {
	static data_t CalculateSize(const std::basic_string<T>& object) {
		return object.size() * sizeof(T);
	}
	static void Load(BlockLoadContext& context, std::basic_string<T>& object) {
		data_t count; context.read(count); object.resize(count); context.read(object.data(), count);
	}
	static void Save(BlockSaveContext& context, const std::basic_string<T>& object) {
		context.write(object.size()); context.write(object.data(), object.size());
	}
};


template<class T>
struct layout_traits<std::vector<T>, std::enable_if_t<has_trivial_layout<T>>> {
	static data_t CalculateSize(const std::vector<T>& object) {
		return object.size() * sizeof(T);
	}
	static void Load(BlockLoadContext& context, std::vector<T>& object) {
		data_t count; context.read(count); object.resize(count); context.read(object.data(), count);
	}
	static void Save(BlockSaveContext& context, const std::vector<T>& object) {
		context.write(object.size()); context.write(object.data(), object.size());
	}
};

template<class T>
struct layout_traits<std::vector<T>, std::enable_if_t<has_custom_layout<T>>> {
	static data_t CalculateSize(const std::vector<T>& object) {
		data_t size = 0; for (auto& item : object) { size += BlockStore::CalculateSize(item); } return size;
	}
	static void Load(BlockLoadContext& context, std::vector<T>& object) {
		data_t count; context.read(count); object.resize(count); for (T& item : object) { BlockStore::Load(context, item); }
	}
	static void Save(BlockSaveContext& context, const std::vector<T>& object) {
		context.write(object.size()); for (const T& item : object) { BlockStore::Save(context, item); }
	}
};


template<class T, size_t count>
struct layout_traits<std::array<T, count>, std::enable_if_t<has_trivial_layout<T>>> {
	static data_t CalculateSize(const std::array<T, count>& object) {
		return count * sizeof(T);
	}
	static void Load(BlockLoadContext& context, std::array<T, count>& object) {
		context.read(object.data(), count);
	}
	static void Save(BlockSaveContext& context, const std::array<T, count>& object) {
		context.write(object.data(), count);
	}
};

template<class T, size_t count>
struct layout_traits<std::array<T, count>, std::enable_if_t<has_custom_layout<T>>> {
	static data_t CalculateSize(const std::array<T, count>& object) {
		data_t size = 0; for (auto& item : object) { size += BlockStore::CalculateSize(item); } return size;
	}
	static void Load(BlockLoadContext& context, std::array<T, count>& object) {
		for (T& item : object) { BlockStore::Load(context, item); }
	}
	static void Save(BlockSaveContext& context, const std::array<T, count>& object) {
		for (const T& item : object) { BlockStore::Save(context, item); }
	}
};


template<class... Ts>
struct layout_traits<std::variant<Ts...>> {
private:
	template<data_t I>
	static std::variant<Ts...> load_variant(BlockLoadContext& context, std::data_t index) {
		if constexpr (I < sizeof...(Ts)) {
			if (index == I) { std::variant_alternative_t<I, std::variant<Ts...>> item; BlockStore::Load(context, item); return item; }
			return load_variant<I + 1>(context, index);
		}
		throw std::runtime_error("invalid variant index");
	}
public:
	static data_t CalculateSize(const std::variant<Ts...>& object) {
		return sizeof(data_t) + std::visit([](auto& item) { return BlockStore::CalculateSize(item); }, object);
	}
	static void Load(BlockLoadContext& context, std::variant<Ts...>& object) {
		data_t index; context.read(index); object = load_variant<0>(context, index);
	}
	static void Save(BlockSaveContext& context, const std::variant<Ts...>& object) {
		context.write(object.index()); std::visit([](auto& item) { BlockStore::Save(context, item); }, object);
	}
};


END_NAMESPACE(BlockStore)