#pragma once

#include "core.h"

#include <memory>


BEGIN_NAMESPACE(BlockStore)

class FileManager;


struct BlockAllocator {
	size_t AddBlock(std::shared_ptr<void> ptr);
	std::shared_ptr<void> GetBlock(size_t index);
	void IncRefBlock(size_t index);
	void DecRefBlock(size_t index);
	void SaveBlock(size_t index, FileManager& manager, size_t block_index);
	size_t IsBlockSaved(size_t index, FileManager& manager);
	void ClearAll();
};


END_NAMESPACE(BlockStore)