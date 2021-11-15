#pragma once

#include "block_layout.h"


BEGIN_NAMESPACE(BlockStore)

class BlockManager;


template<class T>
constexpr void align_offset(data_t& offset) {
	constexpr data_t alignment = sizeof(T) <= 8 ? sizeof(T) : 8;
	static_assert((alignment & (alignment - 1)) == 0);  // 1, 2, 4, 8
	offset = (offset + (alignment - 1)) & ~(alignment - 1);
}

template<class T>
constexpr void align_offset(const byte*& data) {
	data_t offset = data - (byte*)nullptr; align_offset<T>(offset); data = (byte*)nullptr + offset;
}

template<class T>
constexpr void align_offset(byte*& data) {
	data_t offset = data - (byte*)nullptr; align_offset<T>(offset); data = (byte*)nullptr + offset;
}


struct BlockSizeContext {
private:
	data_t size;
public:
	BlockSizeContext() : size(0) {}
public:
	template<class T> void add(const T&) { align_offset<T>(size); size += sizeof(T); }
	template<class T> void add(T object[], data_t count) { align_offset<T>(size); size += sizeof(T) * count; }
public:
	data_t GetSize() const { return size; }
};


struct BlockLoadContext {
private:
	BlockManager& manager;
	const byte* curr;
	const byte* end;
public:
	BlockLoadContext(BlockManager& manager, const byte* begin, data_t length) : manager(manager), curr(begin), end(begin + length) {}
private:
	void CheckNextOffset(const byte* offset) { if (offset > end) { throw std::runtime_error("block size mismatch"); } }
public:
	template<class T>
	void read(T& object) {
		align_offset<T>(curr); const byte* next = curr + sizeof(T); CheckNextOffset(next);
		memcpy(&object, curr, sizeof(T)); curr = next;
	}
	template<class T>
	void read(T object[], data_t count) {
		align_offset<T>(curr); const byte* next = curr + sizeof(T) * count; CheckNextOffset(next);
		memcpy(object, curr, sizeof(T) * count); curr = next;
	}
public:
	BlockManager& GetBlockManager() const { return manager; }
};


struct BlockSaveContext {
private:
	friend class BlockManager;
private:
	BlockManager& manager;
	data_t index;
	byte* begin;
	byte* end;
	byte* curr;
public:
	BlockSaveContext(BlockManager& manager, data_t index, byte* begin, data_t length) :
		manager(manager), index(index), begin(begin), end(begin + length), curr(begin) {
	}
private:
	void CheckNextOffset(const byte* offset) { if (offset > end) { throw std::runtime_error("block size mismatch"); } }
public:
	template<class T>
	void write(const T& object) {
		align_offset<T>(curr); byte* next = curr + sizeof(T); CheckNextOffset(next);
		memcpy(curr, &object, sizeof(T)); curr = next;
	}
	template<class T>
	void write(const T object[], data_t count) {
		align_offset<T>(curr); byte* next = curr + sizeof(T) * count; CheckNextOffset(next);
		memcpy(curr, object, sizeof(T) * count); curr = next;
	}
public:
	BlockManager& GetBlockManager() const { return manager; }
};


END_NAMESPACE(BlockStore)