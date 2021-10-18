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
	operator const T& () const { return *get(); }
	operator const T* () const { return get(); }
};


template<class T>
class BlockRef {
private:
	ref_ptr<BlockManager> manager;
	mutable data_t index;
public:
	BlockRef() : manager(nullptr), index(block_index_invalid) {}
	BlockRef(BlockManager& manager) : manager(&manager), index(block_index_invalid) {}
	BlockRef(const BlockRef<T>& block_ref) { *this = block_ref; }
	~BlockRef();
public:
	BlockRef& operator=(const BlockRef& block_ref);
	BlockManager& GetManager() const { return *manager; }
public:
	BlockPtr<T> Read() const;
	T& Write() const;
private:
	friend struct layout_traits<BlockRef<T>>;
};


END_NAMESPACE(BlockStore)