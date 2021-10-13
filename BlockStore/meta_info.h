#pragma once

#include "core.h"


BEGIN_NAMESPACE(BlockStore)


struct MetaInfo {
	size_t file_size;
	size_t free_block_list_head;
};


END_NAMESPACE(BlockStore)