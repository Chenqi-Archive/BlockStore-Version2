#pragma once

#include "meta_info.h"

#include <memory>
#include <unordered_map>


BEGIN_NAMESPACE(BlockStore)


class FileManager {
public:
	FileManager() {}
	~FileManager() {}
public:
	void Open(std::wstring file) {

	}
	void Close() {}
public:
	void Format() {

	}
private:
	MetaInfo LoadMetaInfo() {}
public:
	template<class T>
	void LoadRootRef(BlockRef<T>& root) { root.manager = this; root.index = LoadMetaInfo().root_index; }
public:
	std::pair<const char*, size_t> LockBlock(size_t index) const {}
	void Append(size_t length) {}
private:
	std::unordered_map<size_t, std::weak_ptr<const void>> block_cache;
protected:
	bool HasBlockPtr(size_t index) { return block_cache.find(index) != block_cache.end(); }
	void SetBlockPtr(size_t index, std::weak_ptr<const void> weak_ptr) { block_cache.emplace(index, weak_ptr); }
	std::shared_ptr<const void> GetBlockPtr(size_t index) { return block_cache.at(index).lock(); }
	void CheckBlockPtr(size_t index) { if (block_cache.at(index).expired()) { block_cache.erase(index); } }
};


END_NAMESPACE(BlockStore)