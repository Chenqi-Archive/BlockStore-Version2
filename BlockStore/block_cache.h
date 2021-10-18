#pragma once

#include "core.h"

#include <memory>
#include <unordered_map>
#include <vector>


BEGIN_NAMESPACE(BlockStore)


class BlockCache {
	// const block cache
private:
	std::unordered_map<data_t, std::weak_ptr<void>> const_block_cache;
public:
	bool IsBlockCached(data_t index) { return const_block_cache.find(index) != const_block_cache.end(); }
	void SetBlock(data_t index, std::weak_ptr<void> weak_ptr) { const_block_cache.emplace(index, weak_ptr); }
	std::shared_ptr<void> GetBlock(data_t index) { return const_block_cache.at(index).lock(); }
	void CheckBlock(data_t index) { if (const_block_cache.at(index).expired()) { const_block_cache.erase(index); } }

	// new block cache
private:
	struct BlockInfo {
		union {
			data_t ref_count;
			data_t index;
			data_t next_index = block_index_invalid;
			data_t const_block_index;
		};
		std::shared_ptr<void> block_data;
	};
	std::vector<BlockInfo> new_block_cache;
	data_t next_index = block_index_invalid;
private:
	BlockInfo& AllocateBlockEntry() {
		if (next_index == block_index_invalid) { next_index = new_block_cache.size(); new_block_cache.emplace_back(); }
		BlockInfo& info = new_block_cache[next_index]; std::swap(info.index, next_index); return info;
	}
	void DeallocateBlockEntry(data_t index) {
		BlockInfo& info = new_block_cache[index]; info.block_data.reset();
		info.next_index = next_index; next_index = index;
	}
private:

public:
	data_t AddNewBlock(std::shared_ptr<void> ptr) {
		BlockInfo& info = AllocateBlockEntry();
		data_t index = info.index; info.block_data = ptr; info.ref_count = 1;
		return index;
	}
	std::shared_ptr<void> GetNewBlock(data_t index) {
		return new_block_cache[index].block_data;
	}
	void IncRefNewBlock(data_t index) {
		++new_block_cache[index].ref_count;
	}
	void DecRefNewBlock(data_t index) {
		if (--new_block_cache[index].ref_count == 0) { DeallocateBlockEntry(index); }
	}
	bool IsNewBlockSaved(data_t index) {
		return new_block_cache[index].block_data == nullptr;
	}
	void SaveNewBlock(data_t index, data_t block_index) {
		new_block_cache[index].const_block_index = block_index;
		new_block_cache[index].block_data.reset();
	}
	data_t GetSavedBlockIndex(data_t index) {
		return new_block_cache[index].const_block_index;
	}
	void ClearNewBlock() {
		new_block_cache.clear(); next_index = block_index_invalid;
	}
};


END_NAMESPACE(BlockStore)