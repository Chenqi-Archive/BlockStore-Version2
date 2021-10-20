#include "block_manager.h"
#include "file_manager.h"
#include "block_cache.h"


BEGIN_NAMESPACE(BlockStore)


BlockManager::BlockManager(std::unique_ptr<FileManager> file) : file(std::move(file)), cache(new BlockCache) { LoadMetaInfo(); }

BlockManager::~BlockManager() {}

void BlockManager::LoadMetaInfo() {
	if (file->GetSize() >= meta_info_size) {
		byte* data = file->Lock(0, meta_info_size);
		memcpy(&meta_info, data, meta_info_size);
	}
}

void BlockManager::SaveMetaInfo() {
	meta_info.file_size = file->GetSize();
	byte* data = file->Lock(0, meta_info_size);
	memcpy(data, &meta_info, meta_info_size);
}

void BlockManager::Format() {
	file->SetSize(meta_info_size);
	meta_info.root_index = block_index_invalid;
	SaveMetaInfo();
}

bool BlockManager::IsBlockCached(data_t index) { return cache->IsBlockCached(index); }
void BlockManager::SetCachedBlock(data_t index, std::weak_ptr<void> weak_ptr) { return cache->SetBlock(index, weak_ptr); }
std::shared_ptr<void> BlockManager::GetCachedBlock(data_t index) { return cache->GetBlock(index); }
void BlockManager::CheckCachedBlock(data_t index) { return cache->CheckBlock(index); }

data_t BlockManager::AddNewBlock(std::shared_ptr<void> ptr) { return convert_new_block_index_from_cache(cache->AddNewBlock(ptr)); }
std::shared_ptr<void> BlockManager::GetNewBlock(data_t index) { return cache->GetNewBlock(convert_new_block_index_to_cache(index)); }
void BlockManager::IncRefNewBlock(data_t index) { return cache->IncRefNewBlock(convert_new_block_index_to_cache(index)); }
void BlockManager::DecRefNewBlock(data_t index) { return cache->DecRefNewBlock(convert_new_block_index_to_cache(index)); }
bool BlockManager::IsNewBlockSaved(data_t index) { return cache->IsNewBlockSaved(convert_new_block_index_to_cache(index)); }
void BlockManager::SaveNewBlock(data_t index, data_t block_index) { return cache->SaveNewBlock(convert_new_block_index_to_cache(index), block_index); }
data_t BlockManager::GetSavedBlockIndex(data_t index) {	return cache->GetSavedBlockIndex(convert_new_block_index_to_cache(index));}
void BlockManager::ClearNewBlock() { return cache->ClearNewBlock(); }

BlockLoadContext BlockManager::LoadBlockContext(data_t index) {
	byte* data_length = file->Lock(index, sizeof(data_t));
	data_t length; memcpy(&length, data_length, sizeof(data_t));
	byte* data_block = file->Lock(index + sizeof(data_t), length);
	return BlockLoadContext(*this, data_block, length);
}

data_t BlockManager::AllocateBlock(data_t size) {
	data_t offset = file->GetSize();
	file->SetSize(offset + sizeof(data_t) + size);
	return offset;
}

BlockSaveContext BlockManager::SaveBlockContext(data_t index, data_t size) {
	byte* data_block = file->Lock(index, sizeof(data_t) + size);
	memcpy(data_block, &size, sizeof(data_t));
	return BlockSaveContext(*this, index, data_block + sizeof(data_t), size);
}

void BlockManager::RenewSaveContext(BlockSaveContext& context) {
	context.data = file->Lock(context.index + sizeof(data_t), context.length);
}


END_NAMESPACE(BlockStore)