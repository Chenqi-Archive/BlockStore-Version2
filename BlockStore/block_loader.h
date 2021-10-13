#pragma once

#include "block_ref.h"

#include <string>
#include <vector>
#include <array>
#include <list>
#include <variant>


BEGIN_NAMESPACE(BlockStore)


class BlockLoader : public BlockManager {
private:
	template<class T>
	void Copy(const char*& data, T& value, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		constexpr size_t size = sizeof(T); memcpy(&value, data, size); data += size;
	}
	template<class T>
	void Copy(const char*& data, T& value, std::enable_if_t<!is_memcpy_initializable<T>, int> = 0) {
		std::apply([&](auto... member) { (Copy(data, value.*member), ...); }, layout(layout_type<T>()));
	}
private:
	template<class T>
	void Load(size_t& index, T& value, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		constexpr size_t size = sizeof(T); memcpy(&value, Read(index, size), size); index += size;
	}
	template<class T>
	void Load(size_t& index, T& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		constexpr size_t size = block_size<T>; const char* data = Read(index, size); Copy(data, value);
	}
	template<class T>
	void Load(size_t& index, T& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		std::apply([&](auto... member) { (Load(index, value.*member), ...); }, layout(layout_type<T>()));
	}
private:
	template<class T, class = std::enable_if_t<std::is_trivial_v<T>>>
	void Load(size_t& index, std::basic_string<T>& value) {
		size_t length; Load(index, length); value.resize(length);
		size_t size = length * sizeof(T); memcpy(value.data(), Read(index, size), size); index += size;
	}
private:
	template<class T>
	void Load(size_t& index, std::vector<T>& value, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		size_t length; Load(index, length); value.resize(length);
		size_t size = length * sizeof(T); memcpy(value.data(), Read(index, size), size); index += size;
	}
	template<class T>
	void Load(size_t& index, std::vector<T>& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		size_t length; Load(index, length); value.resize(length);
		size_t size = length * sizeof(T); const char* data = Read(index, size);
		for (T& item : value) { Copy(data, item); }
	}
	template<class T>
	void Load(size_t& index, std::vector<T>& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		size_t length; Load(index, length); value.resize(length);
		for (T& item : value) { Load(index, item); }
	}
private:
	template<class T, size_t length>
	void Load(size_t& index, std::array<T, length>& value, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		size_t size = length * sizeof(T); memcpy(value.data(), Read(index, size), size); index += size;
	}
	template<class T, size_t length>
	void Load(size_t& index, std::array<T, length>& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		size_t size = length * sizeof(T); const char* data = Read(index, size);
		for (T& item : value) { Copy(data, item); }
	}
	template<class T, size_t length>
	void Load(size_t& index, std::array<T, length>& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		for (T& item : value) { Load(index, item); }
	}
private:
	template<class T>
	void Load(size_t& index, std::list<T>& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		size_t length; Load(index, length); value.resize(length);
		size_t size = length * sizeof(T); const char* data = Read(index, size);
		for (T& item : value) { Copy(data, item); }
	}
	template<class T>
	void Load(size_t& index, std::list<T>& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		size_t length; Load(index, length); value.resize(length);
		for (T& item : value) { Load(index, item); }
	}
private:
	template<size_t I, class... Ts>
	std::variant<Ts...> load_variant(size_t& index, std::size_t tag) {
		if constexpr (I < sizeof...(Ts)) {
			if (tag == I) { return Load<std::variant_alternative_t<I, std::variant<Ts...>>>(index); }
			return load_variant<I + 1, Ts...>(index, tag);
		}
		throw std::runtime_error("invalid variant tag");
	}
	template<class... Ts>
	void Load(size_t& index, std::variant<Ts...>& value) {
		size_t tag = Load<size_t>(index); value = load_variant<0, Ts...>(index, tag);
	}
private:
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
			std::make_unique<T>(static_cast<BlockLoader&>(*manager).LoadBlock<T>(index));
	}
}


END_NAMESPACE(BlockStore)