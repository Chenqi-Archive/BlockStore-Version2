#pragma once

#include "block_ref.h"
#include "stl_helper.h"


BEGIN_NAMESPACE(BlockStore)


struct BlockLoadContext {
private:
	const void* data;
	const size_t length;
	size_t offset;
public:
	const void* data() const { return data + offset; }
public:
	template<class T>
	std::pair<size_t, size_t> align_offset() {
		offset = BlockStore::align_offset<T>(offset);
		size_t size = sizeof(T), next = offset + size; CheckNextOffset(next);
		return { size, next };
	}
	template<class T>
	std::pair<size_t, size_t> align_offset(size_t count) {
		offset = BlockStore::align_offset<T>(offset);
		size_t size = sizeof(T) * count, next = offset + size; CheckNextOffset(next);
		return { size, next };
	}
};


class BlockLoader {
public:
	BlockLoader(FileManager& manager, size_t block_index) {
		std::tie(data, length) = manager.LockBlock(block_index);
	}
private:
	const char* data;
	size_t length;
public:
	template<class T>
	std::shared_ptr<T> LoadBlock() {
		size_t offset = 0; std::shared_ptr<T> block_ptr = std::make_shared<T>(); Load(offset, *block_ptr);
		if (offset != length) { throw std::runtime_error("block size mismatch"); }
		return block_ptr;
	}
private:
	void CheckNextOffset(size_t offset_next) {
		if (offset_next > length) { throw std::runtime_error("block size mismatch"); }
	}
private:
	template<class T>
	std::pair<size_t, size_t> align_offset(size_t& offset) {
		offset = BlockStore::align_offset<T>(offset);
		size_t size = sizeof(T), next = offset + size; CheckNextOffset(next);
		return { size, next };
	}
	template<class T>
	std::pair<size_t, size_t> align_offset(size_t& offset, size_t count) {
		offset = BlockStore::align_offset<T>(offset);
		size_t size = sizeof(T) * count, next = offset + size; CheckNextOffset(next);
		return { size, next };
	}
private:
	template<class T>
	void Load(size_t& offset, T& object, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		auto [size, next] = align_offset<T>(offset); memcpy(&object, data + offset, size); offset = next;
	}
	template<class T>
	void Load(size_t& offset, T& object, std::enable_if_t<!is_memcpy_initializable<T>, int> = 0) {
		std::apply([&](auto... member) { (Load(offset, object.*member), ...); }, layout(layout_type<T>()));
	}
private:
	template<class T1, class T2>
	void Load(size_t& offset, std::pair<T1, T2>& object) {
		Load(offset, object.first); Load(offset, object.second);
	}
	template<class... Ts>
	void Load(size_t& offset, std::tuple<Ts...>& object) {
		std::apply([&](auto&... member) { (Load(offset, member), ...); }, object);
	}
private:
	template<class T, class = std::enable_if_t<std::is_trivial_v<T>>>
	void Load(size_t& offset, std::basic_string<T>& object) {
		size_t count; Load(offset, count); auto [size, next] = align_offset(offset, count);
		object.resize(count); memcpy(object.data(), data + offset, size); offset = next;
	}
private:
	template<class T>
	void Load(size_t& offset, std::vector<T>& object, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		size_t count; Load(offset, count); auto [size, next] = align_offset(offset, count);
		object.resize(count); memcpy(object.data(), data + offset, size); offset = next;
	}
	template<class T>
	void Load(size_t& offset, std::vector<T>& object, std::enable_if_t<!is_memcpy_initializable<T>, int> = 0) {
		size_t count; Load(offset, count); object.resize(count); for (T& item : object) { Load(offset, item); }
	}
private:
	template<class T, size_t count>
	void Load(size_t& offset, std::array<T, count>& object, std::enable_if_t<is_memcpy_initializable<T>, int> = 0) {
		auto [size, next] = align_offset(offset, count); memcpy(object.data(), data + offset, size); offset = next;
	}
	template<class T, size_t count>
	void Load(size_t& offset, std::array<T, count>& object, std::enable_if_t<!is_memcpy_initializable<T>, int> = 0) {
		for (T& item : object) { Load(offset, item); }
	}
private:
	template<class T>
	void Load(size_t& offset, std::list<T>& object) {
		size_t count; Load(offset, count); object.resize(count); for (T& item : object) { Load(offset, item); }
	}
private:
	template<size_t I, class... Ts>
	std::variant<Ts...> load_variant(size_t& offset, std::size_t tag) {
		if constexpr (I < sizeof...(Ts)) {
			if (tag == I) { std::variant_alternative_t<I, std::variant<Ts...>> object; Load(offset, object); return object; }
			return load_variant<I + 1, Ts...>(offset, tag);
		}
		throw std::runtime_error("invalid variant tag");
	}
	template<class... Ts>
	void Load(size_t& offset, std::variant<Ts...>& object) {
		size_t tag; Load(offset, tag); object = load_variant<0, Ts...>(offset, tag);
	}
private:
	template<class T>
	void Load(size_t& offset, BlockRef<T>& block_ref) {
		Load(offset, block_ref.index); block_ref.manager = this;
	}
};


template<class T>
inline BlockPtr<T> BlockRef<T>::LoadBlock() const {
	if (manager == nullptr) { return Create(); }
	if (manager->HasBlockPtr(index)) {
		return BlockPtr<T>(std::static_pointer_cast<const T>(manager->GetBlockPtr(index)), *manager, index);
	} else {
		std::shared_ptr<T> block_ptr = BlockLoader(*manager, index).LoadBlock<T>();
		manager->SetBlockPtr(index, block_ptr); 
		return BlockPtr<T>(block_ptr, *manager, index);
	}
}


END_NAMESPACE(BlockStore)