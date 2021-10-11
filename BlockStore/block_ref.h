#pragma once

#include "block_manager.h"
#include "block_layout.h"

#include <memory>


BEGIN_NAMESPACE(BlockStore)


template<class T>
class BlockRef {
private:
	ref_ptr<BlockManager> manager = nullptr;
	size_t index = invalid_block_index;
	std::unique_ptr<T> resource;
public:
	BlockRef() {}
	BlockRef(BlockManager& manager, size_t index) : manager(&manager), index(index) {}
private:
	void Load();  // defined in block_loader.h
public:
	T& operator*() { Load(); return resource.operator*(); }
};


END_NAMESPACE(BlockStore)