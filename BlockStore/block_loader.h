#pragma once

#include "block_ref.h"

#include <string>
#include <vector>
#include <list>
#include <array>
#include <variant>

#include <algorithm>


BEGIN_NAMESPACE(BlockStore)


class BlockLoader : public BlockManager {
private:
	template<class T, class = std::enable_if_t<is_memory_copyable<T>>>
	void Load(size_t& index, T& value) {
		value = *static_cast<const T*>(Read(index, sizeof(T))); index += sizeof(T);
	}
	template<class T, class = std::enable_if_t<is_memory_copyable_indirect<T>>>
	void Load(size_t& index, T& value) {
		value = *static_cast<const T*>(Read(index, sizeof(T))); index += sizeof(T);
	}
	template<class E, >
	void Load(size_t& index, std::basic_string<E>& value) {
		size_t size; Load(index, size); value.clear(); value.resize(size);
		std::for_each(value.begin(), value.end(), [](T& value) { Load(index, value); });
	}
	template<class T>
	void Load(size_t& index, std::vector<T>& value) {
		size_t size; Load(index, size); value.clear(); value.resize(size);
		memcpy(value.data(), Read(index, ))
			std::for_each(value.begin(), value.end(), [](T& value) { Load(index, value); });
	}
private:
	template<size_t I, class... Ts>
	std::variant<Ts...> load_variant(size_t& index, std::size_t tag) {
		if constexpr (I < sizeof...(Ts)) {
			if (tag == I) { return Load<std::variant_alternative_t<I, std::variant<Ts...>>>(index); }
			return load_variant<I + 1, Ts...>(index, tag);
		}
		throw std::runtime_error("invalid union index");
	}
	template<class... Ts>
	void Load(size_t& index, std::variant<Ts...>& value) {
		size_t tag = Load<size_t>(index); value = load_variant<0, Ts...>(index, tag);
	}
private:
	template<class T, class = std::enable_if_t<!std::is_same_v<void, decltype(layout<T>())>>>
	void Load(size_t& index, T& block) {
		std::apply([](auto... member) { (Load(index, block.*member), ...); }, layout<T>());
	}
	template<class T>
	void Load(size_t& index, BlockRef<T>& block_ref) {
		block_ref = BlockRef<T>(*this, Load<size_t>(index));
	}
private:
	template<class T> T Load(size_t& index) { T t; Load(index, t); return t; }
public:
	template<class T> T LoadBlock(size_t index) { return Load<T>(index); }
	template<class T> T LoadRootBlock() { return LoadBlock<T>(0); }
};


template<class T>
inline void BlockRef<T>::Load() {
	if (resource == nullptr) {
		resource = manager == nullptr ?
			std::make_unique<T>() :
			std::make_unique<T>(static_cast<BlockLoader&>(manager).LoadBlock<T>(index));
	}
}


END_NAMESPACE(BlockStore)