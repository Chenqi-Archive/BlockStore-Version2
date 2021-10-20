#pragma once

#include "core.h"

#include <memory>


BEGIN_NAMESPACE(BlockStore)

class BlockManager;


template<class T>
class BlockPtr : public std::shared_ptr<const T> {
private:
	BlockManager& manager;
	const data_t index;
public:
	BlockPtr(std::shared_ptr<const T> ptr, BlockManager& manager, data_t index) :
		std::shared_ptr<const T>(ptr), manager(manager), index(index) {}
	~BlockPtr();
public:
	operator const T& () const { return *this->get(); }
	operator const T* () const { return this->get(); }
};


template<class T>
class BlockRef {
private:
	ref_ptr<BlockManager> manager;
	mutable data_t index;
public:
	BlockRef() : manager(nullptr), index(block_index_invalid) {}
	BlockRef(BlockManager& manager) : manager(&manager), index(block_index_invalid) {}
	BlockRef(BlockRef&& other) noexcept : manager(other.manager), index(other.index) { other.manager = nullptr; other.index = block_index_invalid; }
	BlockRef(const BlockRef& other);
	~BlockRef();
public:
	void swap(BlockRef& other) { std::swap(manager, other.manager); std::swap(index, other.index); }
	BlockRef& operator=(BlockRef&& other) noexcept { BlockRef temp(other); swap(temp); return *this; }
	BlockRef& operator=(const BlockRef& other) { BlockRef temp(other); swap(temp); return *this; }
public:
	BlockManager& GetManager() const { return *manager; }
	bool operator==(const BlockRef& other) const { return manager == other.manager && index == other.index; }
	bool operator!=(const BlockRef& other) const { return !operator==(other); }
	bool IsEmpty() const { return operator==(BlockRef(GetManager())); }
public:
	BlockPtr<T> Read() const;
	T& Write() const;
private:
	friend class BlockManager;
	friend struct layout_traits<BlockRef>;
};


END_NAMESPACE(BlockStore)