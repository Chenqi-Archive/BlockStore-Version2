#pragma once

#include "block_manager.h"

#include <memory>


BEGIN_NAMESPACE(BlockStore)


template<class T>
class BlockPtr : public std::shared_ptr<T> {
private:
	FileManager& manager;
	size_t index;
public:
	BlockPtr(std::shared_ptr<T> ptr, FileManager& manager, size_t index) : std::shared_ptr<T>(ptr), manager(manager), index(index) {}
	~BlockPtr() { manager->CheckBlockPtr(index); }
};


template<class T>
class BlockRef {
private:
	friend class BlockLoader;
	friend class BlockSaver;
private:
	ref_ptr<FileManager> manager;
	size_t index;
public:
	BlockRef() : manager(nullptr), index(block_index_invalid) {}
	BlockRef(FileManager& manager) : manager(&manager), index(block_index_invalid) {}
	BlockRef(const BlockRef& block_ref) { *this = block_ref; }
	~BlockRef() { if (IsCreated()) { block_allocator.DecRefBlock(index); } }
public:
	void operator=(const BlockRef& block_ref) {
		manager = block_ref.manager; index = block_ref.index;
		if (IsCreated()) { block_allocator.IncRefBlock(index); }
	}
	FileManager& AsParent() const { return *manager; }
private:
	bool IsRaw() const { return manager == nullptr && index == block_index_invalid; }
	bool IsNew() const { return manager != nullptr && index == block_index_invalid; }
	bool IsLoaded() const { return manager != nullptr && index != block_index_invalid; }
private:
	BlockPtr<T> LoadBlock() const;
public:
	BlockPtr<const T> Load() const { return LoadBlock(); }
	std::shared_ptr<T> Create() const {
		if (manager != nullptr) {
			std::shared_ptr<T> block_ptr = LoadBlock();
			index = block_allocator.AddBlock(block_ptr); manager = nullptr;
			return block_ptr;
		} else {
			if (index == block_index_invalid) {
				std::shared_ptr<T> block_ptr = std::make_shared<T>();
				index = block_allocator.AddBlock(block_ptr);
				return block_ptr;
			} else {
				return std::static_pointer_cast<T>(block_allocator.GetBlock(index));
			}
		}
	}
public:
	static constexpr auto _layout() { return declare(&BlockRef::index); }
};

template<class T>
constexpr auto layout(layout_type<BlockRef<T>>) { return BlockRef<T>::_layout(); }


END_NAMESPACE(BlockStore)