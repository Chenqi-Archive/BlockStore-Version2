#pragma once

#include "core.h"

#include <memory>
#include <unordered_map>


BEGIN_NAMESPACE(BlockStore)

class BlockLoader;
class BlockSaver;


class FileManager {
public:
	FileManager() {}
public:
	void Open(std::wstring file) {

	}
	void Close() {}
public:
	void Format() {

	}
private:
	char* data; size_t length;
private:
	void Lock(size_t offset, size_t length) {}
private:
	std::unordered_map<size_t, std::weak_ptr<const void>> block_cache;
private:
	bool HasBlockPtr(size_t index) { return block_cache.find(index) != block_cache.end(); }
	void SetBlockPtr(size_t index, std::weak_ptr<const void> weak_ptr) { block_cache.emplace(index, weak_ptr); }
	std::shared_ptr<const void> GetBlockPtr(size_t index) { return block_cache.at(index).lock(); }
	void CheckBlockPtr(size_t index) { if (block_cache.at(index).expired()) { block_cache.erase(index); } }
public:
	BlockLoader& AsBlockLoader();
	BlockSaver& AsBlockSaver();
};


END_NAMESPACE(BlockStore)