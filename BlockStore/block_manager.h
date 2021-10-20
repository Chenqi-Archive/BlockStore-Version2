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
private:
	bool IsBlockCached(data_t index);
	void SetCachedBlock(data_t index, std::weak_ptr<void> weak_ptr);
	std::shared_ptr<void> GetCachedBlock(data_t index);
	void CheckCachedBlock(data_t index);
private:
	static bool is_new_block_index(data_t index) { return (index & 1) != 0; }
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
		if (std::get_deleter<deleter<T>>(ptr) == nullptr) { throw std::runtime_error("pointer type mismatch"); }
		return std::static_pointer_cast<T>(ptr);
	}
	template<class T>
	static std::shared_ptr<T> pointer_cast(std::shared_ptr<void>&& ptr) {
		if (std::get_deleter<deleter<T>>(ptr) == nullptr) { throw std::runtime_error("pointer type mismatch"); }
		return std::static_pointer_cast<T>(std::move(ptr));
	}

	// load
private:
	BlockLoadContext LoadBlockContext(data_t index);
private:
	template<class T>
	std::shared_ptr<T> LoadBlock(data_t index) {
		std::shared_ptr<T> block(new T(), deleter<T>());
		BlockLoadContext context = LoadBlockContext(index); Load(context, *block);
		return block;
	}
	template<class T>
	std::shared_ptr<T> GetCachedBlock(data_t index) {
		return pointer_cast<T>(GetCachedBlock(index));
	}
	template<class T>
	std::shared_ptr<T> GetBlock(data_t index) {
		if (IsBlockCached(index)) {
			return GetCachedBlock<T>(index);
		} else {
			std::shared_ptr<T> block = LoadBlock<T>(index);
			SetCachedBlock(index, block);
			return block;
		}
	}
private:
	template<class T>
	BlockPtr<const T> ReadBlock(data_t index) {
		if (index != block_index_invalid) {
			if (is_const_block_index(index)) { return BlockPtr<const T>(GetBlock<T>(index), *this, index); }
			if (is_new_block_index(index)) { return BlockPtr<const T>(GetNewBlock<T>(index), *this, index); }
		}
		throw std::runtime_error("invalid block index");
	}
private:
	template<class T>
	void LoadBlockRef(BlockRef<T>& block, data_t index) {
		if (!is_const_block_index(index)) { throw std::runtime_error("invalid block index"); }
		block.manager = this; block.index = index;
	}
public:
	template<class T>
	void LoadRootRef(BlockRef<T>& root) {
		LoadBlockRef(root, meta_info.root_index);
	}

	// create
private:
	template<class T>
	std::shared_ptr<T> CreateNewBlock(data_t& index) {
		std::shared_ptr<T> block_ptr;
		if (is_const_block_index(index)) {
			if (IsBlockCached(index)) {
				block_ptr.reset(new T(*GetCachedBlock<T>(index)), deleter<T>());
			} else {
				block_ptr = LoadBlock<T>(index);
			}
		} else {
			block_ptr.reset(new T(), deleter<T>());
		}
		index = AddNewBlock(block_ptr);
		return block_ptr;
	}
	template<class T>
	std::shared_ptr<T> GetNewBlock(data_t index) {
		return pointer_cast<T>(GetNewBlock(index));
	}
private:
	template<class T>
	BlockPtr<T> WriteBlock(data_t& index) {
		if (index != block_index_invalid) {
			if (is_const_block_index(index)) { return CreateNewBlock<T>(index); }
			if (is_new_block_index(index)) { return GetNewBlock<T>(index); }
		}
		throw std::runtime_error("invalid block index");
	}

	// save
private:
	data_t AllocateBlock(data_t size);
	BlockSaveContext SaveBlockContext(data_t index, data_t size);
	void RenewSaveContext(BlockSaveContext& context);
private:
	bool IsNewBlock(data_t& index) {
		if (index != block_index_invalid) {
			if (is_const_block_index(index)) { return false; }
			if (is_new_block_index(index)) { 
				if (IsNewBlockSaved(index)) { index = GetSavedBlockIndex(index); return false; }
				return true;
			}
		}
		throw std::runtime_error("invalid block index");
	}
private:
	template<class T>
	void SaveBlock(data_t& index) {
		if (!IsNewBlock(index)) { return; }
		std::shared_ptr<T> block = GetNewBlock<T>(index);
		BlockSizeContext size_context; Size(size_context, *block);
		data_t block_size = size_context.GetSize(); align_offset<data_t>(block_size);
		data_t block_index = AllocateBlock(block_size);
		SaveNewBlock(index, block_index);
		index = block_index;
		BlockSaveContext context = SaveBlockContext(block_index, block_size);
		Save(context, *block);
	}
public:
	template<class T>
	void SaveRootRef(BlockRef<T>& root) {
		if (root.manager != this) { throw std::invalid_argument("block manager mismatch"); }
		SaveBlock<T>(root.index);
		meta_info.root_index = root.index;
		SaveMetaInfo();
		ClearNewBlock();
	}

private:
	template<class> friend class BlockPtr;
	template<class> friend class BlockRef;
	template<class, class> friend struct layout_traits;
};


template<class T>
inline BlockPtr<const T>::~BlockPtr() {
	this->reset(); manager.CheckCachedBlock(index);
}

template<class T>
inline BlockRef<T>::BlockRef(BlockManager& manager) : manager(&manager), index(block_index_invalid) {
	manager.CreateNewBlock<T>(index);
}

template<class T>
inline BlockRef<T>::BlockRef(const BlockRef& block_ref) : manager(block_ref.manager), index(block_ref.index) {
	if (manager != nullptr && manager->IsNewBlock(index)) { manager->IncRefNewBlock(index); }
}

template<class T>
inline BlockRef<T>::~BlockRef() {
	if (manager != nullptr && manager->IsNewBlock(index)) { manager->DecRefNewBlock(index); }
}

template<class T>
inline BlockPtr<const T> BlockRef<T>::Read() const {
	if (manager == nullptr) { throw std::invalid_argument("block ref uninitialized"); }
	return manager->ReadBlock<T>(index);
}

template<class T>
inline BlockPtr<T> BlockRef<T>::Write() const {
	if (manager == nullptr) { throw std::invalid_argument("block ref uninitialized"); }
	return manager->WriteBlock<T>(index);
}


template<class T>
struct layout_traits<BlockRef<T>> {
	static void Size(BlockSizeContext& context, const BlockRef<T>& object) {
		context.add(object.index);
	}
	static void Load(BlockLoadContext& context, BlockRef<T>& object) {
		data_t index; context.read(index); context.GetBlockManager().LoadBlockRef(object, index);
	}
	static void Save(BlockSaveContext& context, const BlockRef<T>& object) {
		if (&context.GetBlockManager() != object.manager) { throw std::invalid_argument("block manager mismatch"); }
		object.manager->SaveBlock<T>(object.index); object.manager->RenewSaveContext(context);
		context.write(object.index);
	}
};


END_NAMESPACE(BlockStore)