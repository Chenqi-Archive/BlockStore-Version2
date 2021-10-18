#pragma once

#include "meta_info.h"
#include "block_traits.h"
#include "block_ref.h"

#include <memory>


BEGIN_NAMESPACE(BlockStore)

class FileManager;
class BlockCache;


class BlockManager {
public:
	BlockManager(std::unique_ptr<FileManager> file);
	~BlockManager();

private:
	std::unique_ptr<FileManager> file;

	// meta
private:
	MetaInfo meta_info;
private:
	void LoadMetaInfo();
	void SaveMetaInfo();
public:
	void Format();

	// cache
private:
	std::unique_ptr<BlockCache> cache;
private:
	static bool is_const_block_index(data_t index) { return index % sizeof(data_t) == 0; }
	static bool is_new_block_index(data_t index) { return index & 1 != 0; }
private:
	bool IsBlockCached(data_t index);
	void SetBlock(data_t index, std::weak_ptr<void> weak_ptr);
	std::shared_ptr<void> GetBlock(data_t index);
	void CheckBlock(data_t index);
private:
	static data_t convert_new_block_index_from_cache(data_t index) { return index * 2 + 1; }
	static data_t convert_new_block_index_to_cache(data_t index) { return index / 2; }
private:
	data_t AddNewBlock(std::shared_ptr<void> ptr);
	std::shared_ptr<void> GetNewBlock(data_t index);
	void IncRefNewBlock(data_t index);
	void DecRefNewBlock(data_t index);
	bool IsNewBlockSaved(data_t index);
	void SaveNewBlock(data_t index, data_t block_index);
	data_t GetSavedBlockIndex(data_t index);
	void ClearNewBlock();

	// helper
private:
	template<class T>
	struct deleter {
		void operator()(T* object) { delete object; }
	};
	template<class T>
	static std::shared_ptr<T> pointer_cast(const std::shared_ptr<void>& ptr) {
		if (std::get_deleter<deleter<T>>(ptr) == nullptr) { throw std::invalid_argument("pointer type mismatch"); }
		return std::static_pointer_cast<T>(ptr);
	}
	template<class T>
	static std::shared_ptr<T> pointer_cast(std::shared_ptr<void>&& ptr) {
		if (std::get_deleter<deleter<T>>(ptr) == nullptr) { throw std::invalid_argument("pointer type mismatch"); }
		return std::static_pointer_cast<T>(std::move(ptr));
	}

	// load
private:
	BlockLoadContext LoadBlockContext(data_t index);
private:
	template<class T>
	std::shared_ptr<T> LoadBlock(data_t index) {
		if (IsBlockCached(index)) {
			return pointer_cast<T>(GetBlock(index));
		} else {
			std::shared_ptr<T> block_ptr(new T(), deleter<T>());
			BlockLoadContext context = LoadBlockContext(index);
			Load(context, *block_ptr);
			SetBlock(index, block_ptr);
			return block_ptr;
		}
	}
private:
	template<class T>
	BlockPtr<T> ReadBlock(data_t& index) {
		if (is_new_block_index(index)) { return BlockPtr<T>(GetNewBlock<T>(index), *this, index); }
		if (is_const_block_index(index)) { return BlockPtr<T>(LoadBlock<T>(index), *this, index); }
		throw std::runtime_error("invalid block index");
	}
public:
	template<class T>
	void LoadRootRef(BlockRef<T>& root) {
		root.manager = this; root.index = meta_info.root_index;
	}

	// create
private:
	template<class T>
	std::shared_ptr<T> CreateNewBlock(data_t& index) {
		if (is_const_block_index(index)) {

		}
		std::shared_ptr<T> block_ptr(new T(), deleter<T>());
		index = AddNewBlock(block_ptr);
		return block_ptr;
	}
	template<class T>
	std::shared_ptr<T> GetNewBlock(data_t index) {
		return pointer_cast<T>(GetNewBlock(index));
	}
private:
	template<class T>
	T& WriteBlock(data_t& index) {
		if (index == block_index_invalid || is_const_block_index(index)) { return *CreateNewBlock<T>(index); }
		if (is_new_block_index(index)) { return *GetNewBlock<T>(index); }
		throw std::runtime_error("invalid block index");
	}

	// save
private:
	data_t AllocateBlock(data_t size);
	BlockSaveContext SaveBlockContext(data_t index, data_t size);
private:
	bool CheckNewBlock(data_t& index) {
		if (is_const_block_index(index)) { return false; }
		if (IsNewBlockSaved(index)) { index = GetSavedBlockIndex(index); return false; }
		return true;
	}
private:
	template<class T>
	void SaveBlock(data_t& index) {
		if (!CheckNewBlock(index)) { return; }
		std::shared_ptr<T> block = pointer_cast<T>(GetNewBlock(index));
		data_t size = CalculateSize(block);
		data_t block_index = AllocateBlock(size);
		SaveNewBlock(index, block_index);
		index = block_index;
		BlockSaveContext context = SaveBlockContext(block_index, size);
		Save(context, *block);
	}
public:
	template<class T>
	void SaveRootRef(BlockRef<T>& root) {
		if (root.manager != this) { throw std::invalid_argument("block manager mismatch"); }
		SaveBlock<T>(root.index);
		meta_info.root_index = root;
		SaveMetaInfo();
		ClearNewBlock();
	}

private:
	template<class> friend struct BlockPtr;
	template<class> friend struct BlockRef;
	template<class, class> friend struct layout_traits;
};


template<class T>
inline BlockPtr<T>::~BlockPtr() {
	manager.CheckBlock(index);
}

template<class T>
inline BlockRef<T>::~BlockRef() {
	if (manager != nullptr && manager->CheckNewBlock(index)) { manager->DecRefNewBlock(index); }
}

template<class T>
inline BlockRef<T>& BlockRef<T>::operator=(const BlockRef<T>& block_ref) {
	manager = block_ref.manager; index = block_ref.index;
	if (manager != nullptr && manager->CheckNewBlock(index)) { manager->IncRefNewBlock(index); }
	return *this;
}

template<class T>
inline BlockPtr<T> BlockRef<T>::Read() const {
	if (manager == nullptr) { throw std::logic_error("block ref uninitialized"); }
	return manager->ReadBlock(index);
}

template<class T>
inline T& BlockRef<T>::Write() const {
	if (manager == nullptr) { throw std::logic_error("block ref uninitialized"); }
	return manager->WriteBlock(index);
}


template<class T>
struct layout_traits<BlockRef<T>> {
	static data_t CalculateSize(const BlockRef<T>& object) { return sizeof(data_t); }
	static void Load(BlockLoadContext& context, BlockRef<T>& object) {
		context.read(object.index); object.manager = &context.GetBlockManager();
	}
	static void Save(BlockSaveContext& context, const BlockRef<T>& object) {
		if (&context.GetBlockManager() != object.manager) { throw std::invalid_argument("block manager mismatch"); }
		object.manager->SaveBlock(object.index); context.write(object.index);
	}
};


END_NAMESPACE(BlockStore)