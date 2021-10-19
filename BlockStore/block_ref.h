#pragma once

#include "core.h"

#include <memory>


BEGIN_NAMESPACE(BlockStore)

class BlockManager;


template<class T>
class BlockPtr : public std::shared_ptr<const T> {
private:
	BlockManager& manager;
	data_t index;
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
	BlockRef(BlockRef&& block_ref) : manager(block_ref.manager), index(block_ref.index) { block_ref.manager = nullptr; block_ref.index = block_index_invalid; }
	BlockRef(const BlockRef& block_ref);
	~BlockRef();
public:
	void swap(BlockRef& block_ref) { std::swap(manager, block_ref.manager); std::swap(index, block_ref.index); }
public:
	BlockRef& operator=(const BlockRef& block_ref) { BlockRef temp(block_ref); swap(temp); return *this; }
	BlockManager& GetManager() const { return *manager; }
public:
	BlockPtr<T> Read() const;
	T& Write() const;
private:
	friend class BlockManager;
	friend struct layout_traits<BlockRef>;
};


END_NAMESPACE(BlockStore)