#pragma once

#include "meta_info.h"

#include <memory>


BEGIN_NAMESPACE(BlockStore)


class BlockManager {
public:
	BlockManager() {}

	// file
private:
	size_t GetSize() const {}
	void SetSize(size_t size) {}
	void* Lock(size_t offset, size_t length) {}

	// meta
private:
	MetaInfo meta_info;
public:
	size_t GetRootIndex() { return meta_info.root_index; }

	// block
private:
	BlockLoadContext LoadBlock(size_t index) {
		if (index % 8 != 0) { throw std::invalid_argument(); }
		Lock(index, )
	}



};


END_NAMESPACE(BlockStore)