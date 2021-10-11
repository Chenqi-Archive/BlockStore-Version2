#pragma once

#include "block_manager.h"


BEGIN_NAMESPACE(BlockStore)



class BlockManagerFile : public BlockManager {
public:
	BlockManagerFile(std::wstring file) {}
private:
	virtual const void* Read(size_t index, size_t length) pure;
};


END_NAMESPACE(BlockStore)