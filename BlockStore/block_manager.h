#pragma once

#include "core.h"


BEGIN_NAMESPACE(BlockStore)


class BlockManager {
	friend class BlockLoader;
	virtual const char* Read(size_t index, size_t length) pure;
	virtual char* Write(size_t index, size_t length) pure;
};


END_NAMESPACE(BlockStore)