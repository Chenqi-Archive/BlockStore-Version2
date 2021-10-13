#pragma once

#include "block_manager.h"
#include "block_layout.h"

#include <memory>


BEGIN_NAMESPACE(BlockStore)

constexpr size_t block_index_invalid = -1;


template<class T>
class BlockRef {
private:
	ref_ptr<BlockManager> manager = nullptr;
	size_t index = block_index_invalid;
	std::unique_ptr<T> resource;
public:
	BlockRef() {}
	BlockRef(BlockManager& manager, size_t index) : manager(&manager), index(index) {}
private:
	void Load();  // defined in block_loader.h
	void Store();  // defined in block_storer.h
public:
	T& operator*() { Load(); return resource.operator*(); }
};


END_NAMESPACE(BlockStore)