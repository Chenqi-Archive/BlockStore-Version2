#pragma once

#include "block_ref.h"

#include <string>
#include <vector>
#include <array>
#include <list>
#include <variant>


BEGIN_NAMESPACE(BlockStore)


class BlockSaver : public FileManager {
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
	void Save(size_t& index, const T& value, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		constexpr size_t size = sizeof(T); memcpy(Write(index, size), &value, size); index += size;
	}
	template<class T>
	void Save(size_t& index, const T& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		constexpr size_t size = block_size<T>; char* data = Write(index, size); Copy(data, value);
	}
	template<class T>
	void Save(size_t& index, const T& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		std::apply([&](auto... member) { (Save(index, value.*member), ...); }, layout(layout_type<T>()));
	}
private:
	template<class T, class = std::enable_if_t<std::is_trivial_v<T>>>
	void Save(size_t& index, const std::basic_string<T>& value) {
		size_t length; Save(index, length); value.resize(length);
		size_t size = length * sizeof(T); memcpy(Write(index, size), value.data(), size); index += size;
	}
private:
	template<class T>
	void Save(size_t& index, const std::vector<T>& value, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		size_t length; Save(index, length); value.resize(length);
		size_t size = length * sizeof(T); memcpy(Write(index, size), value.data(), size); index += size;
	}
	template<class T>
	void Save(size_t& index, const std::vector<T>& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		size_t length; Save(index, length); value.resize(length);
		size_t size = length * sizeof(T); char* data = Write(index, size);
		for (T& item : value) { Copy(data, item); }
	}
	template<class T>
	void Save(size_t& index, const std::vector<T>& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		size_t length; Save(index, length); value.resize(length);
		for (T& item : value) { Save(index, item); }
	}
private:
	template<class T, size_t length>
	void Save(size_t& index, const std::array<T, length>& value, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		size_t size = length * sizeof(T); memcpy(Write(index, size), value.data(), size); index += size;
	}
	template<class T, size_t length>
	void Save(size_t& index, const std::array<T, length>& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		size_t size = length * sizeof(T); char* data = Write(index, size);
		for (T& item : value) { Copy(data, item); }
	}
	template<class T, size_t length>
	void Save(size_t& index, const std::array<T, length>& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		for (T& item : value) { Save(index, item); }
	}
private:
	template<class T>
	void Save(size_t& index, const std::list<T>& value, std::enable_if_t<is_block_size_fixed<T>, int> = 0) {
		size_t length; Save(index, length); value.resize(length);
		size_t size = length * sizeof(T); char* data = Write(index, size);
		for (T& item : value) { Copy(data, item); }
	}
	template<class T>
	void Save(size_t& index, const std::list<T>& value, std::enable_if_t<is_block_size_dynamic<T>, int> = 0) {
		size_t length; Save(index, length); value.resize(length);
		for (T& item : value) { Save(index, item); }
	}
private:
	template<class... Ts>
	void Save(size_t& index, const std::variant<Ts...>& value) {
		Save(index, value.index()); std::visit([&](auto value) { Save(index, value); }, value);
	}
private:
	template<class T>
	void Save(size_t& index, const BlockRef<T>& block_ref) {
		if (block_ref.IsLoaded()) { return; }
		block_ref = block_ref = BlockRef<T>(*this, SaveBlock(*block_ref.Create()));
	}
private:
	template<class T>
	size_t Save(const T& block) {
		size_t index = GetEndOffset(), offset = index; Save(offset, block); return index;
	}
private:
	template<class T> size_t SaveBlock(const T& block) { return Save(block); }
public:
	template<class T>
	size_t SaveRootRef(const BlockRef<T>& root) {
		
		return SaveBlock(root); 
	}
};


BlockSaver& FileManager::AsBlockSaver() { return static_cast<BlockSaver&>(*this); }


END_NAMESPACE(BlockStore)