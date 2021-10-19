#pragma once

#include "block_layout.h"


BEGIN_NAMESPACE(BlockStore)

class BlockManager;


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
	const byte* const data;
	const data_t length;
	data_t offset;
public:
	BlockLoadContext(BlockManager& manager, const byte* data, data_t length) : manager(manager), data(data), length(length), offset(0) {}
private:
	void CheckNextOffset(data_t offset) { if (offset > length) { throw std::runtime_error("block size mismatch"); } }
public:
	template<class T>
	void read(T& object) {
		align_offset<T>(offset); data_t next = offset + sizeof(T); CheckNextOffset(next);
		memcpy(&object, data + offset, sizeof(T)); offset = next;
	}
	template<class T>
	void read(T object[], data_t count) {
		align_offset<T>(offset); data_t next = offset + sizeof(T) * count; CheckNextOffset(next);
		memcpy(object, data + offset, sizeof(T) * count); offset = next;
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
	byte* data;
	const data_t length;
	data_t offset;
public:
	BlockSaveContext(BlockManager& manager, data_t index, byte* data, data_t length) : manager(manager), index(index), data(data), length(length), offset(0) {}
private:
	void CheckNextOffset(data_t offset) { if (offset > length) { throw std::runtime_error("block size mismatch"); } }
public:
	template<class T>
	void write(const T& object) {
		align_offset<T>(offset); data_t next = offset + sizeof(T); CheckNextOffset(next);
		memcpy(data + offset, &object, sizeof(T)); offset = next;
	}
	template<class T>
	void write(const T object[], data_t count) {
		align_offset<T>(offset); data_t next = offset + sizeof(T) * count; CheckNextOffset(next);
		memcpy(data + offset, object, sizeof(T) * count); offset = next;
	}
public:
	BlockManager& GetBlockManager() const { return manager; }
};


END_NAMESPACE(BlockStore)