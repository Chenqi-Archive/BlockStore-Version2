#pragma once

#include "block_layout.h"


BEGIN_NAMESPACE(BlockStore)


template<class T>
constexpr size_t GetBlockSize() { return 0; }

template<class T, std::enable_if_t<std::is_arithmetic_v<T>>>
constexpr size_t GetBlockSize() { return sizeof(T); }





END_NAMESPACE(BlockStore)