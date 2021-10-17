#pragma once

#include "block_ref.h"
#include "stl_helper.h"


BEGIN_NAMESPACE(BlockStore)


class BlockLoader {
public:
	template<class T>
	std::shared_ptr<T> LoadBlock() {
		size_t offset = 0; std::shared_ptr<T> block_ptr = std::make_shared<T>(); Load(offset, *block_ptr);
		if (offset != length) { throw std::runtime_error("block size mismatch"); }
		return block_ptr;
	}
private:
	template<class T>
	void Load(size_t& offset, BlockRef<T>& block_ref) {
		Load(offset, block_ref.index); block_ref.manager = this;
	}
};


template<class T>
inline BlockPtr<T> BlockRef<T>::LoadBlock() const {
	if (manager == nullptr) { return Create(); }
	if (manager->HasBlockPtr(index)) {
		return BlockPtr<T>(std::static_pointer_cast<const T>(manager->GetBlockPtr(index)), *manager, index);
	} else {
		std::shared_ptr<T> block_ptr = BlockLoader(*manager, index).LoadBlock<T>();
		manager->SetBlockPtr(index, block_ptr); 
		return BlockPtr<T>(block_ptr, *manager, index);
	}
}


END_NAMESPACE(BlockStore)