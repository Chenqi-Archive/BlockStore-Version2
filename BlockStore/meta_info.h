#pragma once

#include "core.h"


BEGIN_NAMESPACE(BlockStore)


struct MetaInfo {
	data_t file_size = 0;
	data_t root_index = block_index_invalid;
};

constexpr data_t meta_info_size = sizeof(MetaInfo);


END_NAMESPACE(BlockStore)