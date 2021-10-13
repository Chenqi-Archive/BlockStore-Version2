#pragma once

#include "file_manager.h"
#include "block_allocator.h"
#include "block_layout.h"

#include <memory>


BEGIN_NAMESPACE(BlockStore)


constexpr size_t block_index_invalid = -1;


template<class T>
class BlockPtr : public std::shared_ptr<T> {
private:
	ref_ptr<FileManager> manager;
	size_t index;
public:
	BlockPtr(std::shared_ptr<T> ptr) : std::shared_ptr<T>(ptr), manager(nullptr), index(block_index_invalid) {}
	BlockPtr(std::shared_ptr<T> ptr, FileManager& manager, size_t index) : std::shared_ptr<T>(ptr), manager(&manager), index(index) {}
	~BlockPtr() { if (manager != nullptr) { manager->CheckBlockPtr(index); } }
};


template<class T>
class BlockRef {
private:
	ref_ptr<FileManager> manager = nullptr;
	size_t index = block_index_invalid;
public:
	BlockRef() {}
	BlockRef(FileManager& manager, size_t index) : manager(&manager), index(index) {}
	~BlockRef() { if (HasAllocatedBlock()) { block_allocator.DerefBlock(index); } }
private:
	bool HasAllocatedBlock() const { return manager == nullptr && index != block_index_invalid; }
private:
	std::shared_ptr<T> LoadBlock();  // defined in block_loader.h
public:
	BlockPtr<const T> Load() const {
		if (manager == nullptr) { return Create(); }
		if (manager->HasBlockPtr(index)) {
			return BlockPtr<T>(std::static_pointer_cast<const T>(manager->GetBlockPtr(index)), *manager, index);
		} else {
			std::shared_ptr<const T> block_ptr = LoadBlock(); manager->SetBlockPtr(index, block_ptr);
			return BlockPtr<T>(block_ptr, *manager, index);
		}
	}
	BlockPtr<T> Create() const {
		if (manager != nullptr) {
			std::shared_ptr<T> block_ptr = LoadBlock();
			index = block_allocator.AddBlock(block_ptr); manager = nullptr;
			return BlockPtr<T>(block_ptr);
		} else {
			if (index == block_index_invalid) { 
				std::shared_ptr<T> block_ptr = std::make_shared<T>();
				index = block_allocator.AddBlock(block_ptr);
				return BlockPtr<T>(block_ptr);
			} else {
				return BlockPtr<T>(std::static_pointer_cast<T>(block_allocator.GetBlock(index)));
			}
		}
	}
private:
	void Save(const T& value);
public:
	static constexpr auto _layout() { return declare(&BlockRef::index); }
};

template<class T>
constexpr auto layout(layout_type<BlockRef<T>>) { return BlockRef<T>::_layout(); }


END_NAMESPACE(BlockStore)