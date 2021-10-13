#pragma once

#include "core.h"

#include <memory>


BEGIN_NAMESPACE(BlockStore)


struct BlockAllocator {
	size_t AddBlock(std::shared_ptr<void> ptr);
	std::shared_ptr<void> GetBlock(size_t index);
	void DerefBlock(size_t index);
	void ClearAll();
};

static BlockAllocator block_allocator;


END_NAMESPACE(BlockStore)