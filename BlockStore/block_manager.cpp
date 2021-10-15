#include "block_allocator.h"

#include <vector>


BEGIN_NAMESPACE(BlockStore)

BEGIN_NAMESPACE(Anonymous)

constexpr size_t block_index_invalid = -1;


struct BlockInfo {
	union {
		size_t ref_count;
		size_t index;
		size_t next_index = block_index_invalid;
	};
	std::shared_ptr<void> block_data;
};

std::vector<BlockInfo> block_cache;

size_t next_index = block_index_invalid;


BlockInfo& AllocateBlockEntry() {
	if (next_index == block_index_invalid) { next_index = block_cache.size(); block_cache.emplace_back(); }
	BlockInfo& info = block_cache[next_index]; std::swap(info.index, next_index); return info;
}

void DeallocateBlockEntry(size_t index) {
	BlockInfo& info = block_cache[index]; info.block_data.reset();
	info.next_index = next_index; next_index = index;
}


struct BlockSaveInfo {
	ref_ptr<FileManager> manager;
	size_t index;
};

std::vector<BlockSaveInfo> block_save_info;


END_NAMESPACE(Anonymous)


size_t BlockAllocator::AddBlock(std::shared_ptr<void> ptr) {
	BlockInfo& info = AllocateBlockEntry();
	size_t index = info.index; info.block_data = ptr; info.ref_count = 1;
	return info.index;
}

std::shared_ptr<void> BlockAllocator::GetBlock(size_t index) {
	return block_cache[index].block_data;
}

void BlockAllocator::IncRefBlock(size_t index) {
	++block_cache[index].ref_count;
}

void BlockAllocator::DecRefBlock(size_t index) {
	if (--block_cache[index].ref_count == 0) { DeallocateBlockEntry(index); }
}

void BlockAllocator::SaveBlock(size_t index, FileManager& manager, size_t block_index) {
}

size_t BlockAllocator::IsBlockSaved(size_t index, FileManager& manager) {
	return size_t();
}

void BlockAllocator::ClearAll() {
	block_cache.clear(); next_index = block_index_invalid;
}


END_NAMESPACE(BlockStore)