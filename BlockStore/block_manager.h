#pragma once

#include "core.h"


BEGIN_NAMESPACE(BlockStore)


class BlockManager {
	friend class BlockLoader;
	virtual const void* Read(size_t index, size_t length) pure;
};


END_NAMESPACE(BlockStore)