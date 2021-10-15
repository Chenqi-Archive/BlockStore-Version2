#pragma once

#include "core.h"


BEGIN_NAMESPACE(BlockStore)


constexpr size_t block_index_invalid = -1;


struct MetaInfo {
	size_t root_index = block_index_invalid;
};


END_NAMESPACE(BlockStore)