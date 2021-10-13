#pragma once

#include "block_ref.h"

#include <string>
#include <vector>
#include <array>
#include <list>
#include <variant>


BEGIN_NAMESPACE(BlockStore)


class BlockStorer : public BlockManager {
private:
	template<class T>
	void Copy(char*& data, const T& value, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		constexpr size_t size = sizeof(T); memcpy(data, &value, size); data += size;
	}
	template<class T>
	void Copy(char*& data, const T& value, std::enable_if_t<!is_memcpy_initializable<T>, int> = 0) {
		std::apply([&](auto... member) { (Copy(data, value.*member), ...); }, layout(layout_type<T>()));
	}
private:
	template<class T>
	void Store(size_t& index, const T& value, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		constexpr size_t size = sizeof(T); memcpy(Write(index, size), &value, size); index += size;
	}
	template<class T>
	void Store(size_t& index, const T& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		constexpr size_t size = block_size<T>; char* data = Write(index, size); Copy(data, value);
	}
	template<class T>
	void Store(size_t& index, const T& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		std::apply([&](auto... member) { (Store(index, value.*member), ...); }, layout(layout_type<T>()));
	}
private:
	template<class T, class = std::enable_if_t<std::is_trivial_v<T>>>
	void Store(size_t& index, const std::basic_string<T>& value) {
		size_t length; Store(index, length); value.resize(length);
		size_t size = length * sizeof(T); memcpy(Write(index, size), value.data(), size); index += size;
	}
private:
	template<class T>
	void Store(size_t& index, const std::vector<T>& value, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		size_t length; Store(index, length); value.resize(length);
		size_t size = length * sizeof(T); memcpy(Write(index, size), value.data(), size); index += size;
	}
	template<class T>
	void Store(size_t& index, const std::vector<T>& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		size_t length; Store(index, length); value.resize(length);
		size_t size = length * sizeof(T); char* data = Write(index, size);
		for (T& item : value) { Copy(data, item); }
	}
	template<class T>
	void Store(size_t& index, const std::vector<T>& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		size_t length; Store(index, length); value.resize(length);
		for (T& item : value) { Store(index, item); }
	}
private:
	template<class T, size_t length>
	void Store(size_t& index, const std::array<T, length>& value, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		size_t size = length * sizeof(T); memcpy(Write(index, size), value.data(), size); index += size;
	}
	template<class T, size_t length>
	void Store(size_t& index, const std::array<T, length>& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		size_t size = length * sizeof(T); char* data = Write(index, size);
		for (T& item : value) { Copy(data, item); }
	}
	template<class T, size_t length>
	void Store(size_t& index, const std::array<T, length>& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		for (T& item : value) { Store(index, item); }
	}
private:
	template<class T>
	void Store(size_t& index, const std::list<T>& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		size_t length; Store(index, length); value.resize(length);
		size_t size = length * sizeof(T); char* data = Write(index, size);
		for (T& item : value) { Copy(data, item); }
	}
	template<class T>
	void Store(size_t& index, const std::list<T>& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		size_t length; Store(index, length); value.resize(length);
		for (T& item : value) { Store(index, item); }
	}
private:
	template<class... Ts>
	void Store(size_t& index, const std::variant<Ts...>& value) {
		Store(index, value.index()); std::visit([&](auto value) { Store(index, value); }, value);
	}
private:
#error
	template<class T>
	void Store(size_t& index, const BlockRef<T>& block_ref) {
		block_ref = BlockRef<T>(*this, Store<size_t>(index));
	}
public:
	template<class T> T StoreBlock(size_t index) { return Store<T>(index); }
	template<class T> T StoreRootBlock() { return StoreBlock<T>(0); }
};


template<class T>
inline void BlockRef<T>::Store() {
	if (resource == nullptr) {
		resource = manager == nullptr ?
			std::make_unique<T>() :
			std::make_unique<T>(static_cast<BlockStoreer&>(*manager).StoreBlock<T>(index));
	}
}


END_NAMESPACE(BlockStore)