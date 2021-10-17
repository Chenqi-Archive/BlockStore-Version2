#pragma once

#include "uncopyable.h"


BEGIN_NAMESPACE(BlockStore)


class FileManager : Uncopyable {
public:
	enum class CreateMode : uint {  // |  existing	 | not existing	|  
		CreateNew = 1,				// | 	ERROR	 |	  create	|
		CreateAlways = 2,			// | 	clear	 |	  create	|
		OpenExisting = 3,			// | 	open	 |	  ERROR		|
		OpenAlways = 4,				// | 	open	 |	  create	|
		TruncateExisting = 5		// | 	clear	 |	  ERROR		|
	};
	enum class AccessMode : uint {
		ReadOnly = 0x80000000L,		// GENERIC_READ
		ReadWrite = 0xC0000000L,	// GENERIC_READ | GENERIC_WRITE
	};
	enum class ShareMode : uint {
		None = 0x00000000,			// NULL
		ReadOnly = 0x00000001,		// FILE_SHARE_READ
		ReadWrite = 0x00000003,		// FILE_SHARE_READ | FILE_SHARE_WRITE
	};
public:
	FileManager(const wchar path[],
				CreateMode create_mode = CreateMode::OpenAlways,
				AccessMode access_mode = AccessMode::ReadWrite,
				ShareMode share_mode = ShareMode::None);
	~FileManager();
private:
	using HANDLE = void*;
	HANDLE file;
	uint64 size;
	CreateMode create_mode;
	AccessMode access_mode;
	ShareMode share_mode;
public:
	uint64 GetSize() const { return size; }
	void SetSize(uint64 size);
private:
	HANDLE mapping;
private:
	void DoMapping();
	void UndoMapping();
private:
	byte* view_address;
	uint64 view_offset;
	uint64 view_length;
private:
	void Unlock();
public:
	byte* Lock(uint64 offset, uint64 length);
};


END_NAMESPACE(BlockStore)