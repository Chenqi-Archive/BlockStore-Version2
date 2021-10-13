#pragma once

#include "block_manager.h"


BEGIN_NAMESPACE(BlockStore)



class FileManager : public BlockManager {
public:
	FileManager(std::wstring file) {}
private:
	virtual const char* Read(size_t index, size_t length) override;
	virtual char* Write(size_t index, size_t length) override;
};


END_NAMESPACE(BlockStore)