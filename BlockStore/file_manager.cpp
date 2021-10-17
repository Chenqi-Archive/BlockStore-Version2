#include "file_manager.h"

#include <Windows.h>


BEGIN_NAMESPACE(BlockStore)

BEGIN_NAMESPACE(Anonymous)

static_assert(sizeof(LARGE_INTEGER) == sizeof(uint64));

static const uint64 allocation_granularity = []() { SYSTEM_INFO info; GetSystemInfo(&info); return info.dwAllocationGranularity; }();

struct Interval {
	uint64 begin;
	uint64 length;

	Interval(uint64 begin, uint64 length) : begin(begin), length(length) {
		if (left() > right()) { throw std::invalid_argument("invalid interval"); }
	}

	uint64 left() const { return begin; }
	uint64 right() const { return begin + length; }

	bool Contains(const Interval& interval) const { return interval.left() >= left() && interval.right() <= right(); }
};

uint64 align_offset_floor(uint64 offset, uint64 alignment) {
	assert((alignment & (alignment - 1)) == 0);  // power of 2
	return offset & ~(alignment - 1);
}

uint64 align_offset_ceil(uint64 offset, uint64 alignment) {
	assert((alignment & (alignment - 1)) == 0);  // power of 2
	return (offset + (alignment - 1)) & ~(alignment - 1);
}

END_NAMESPACE(Anonymous)


FileManager::FileManager(const wchar path[], CreateMode create_mode, AccessMode access_mode, ShareMode share_mode) :
	file(INVALID_HANDLE_VALUE), size(0), create_mode(create_mode), access_mode(access_mode), share_mode(share_mode),
	mapping(NULL), view_address(nullptr), view_offset(0), view_length(0) {
	file = CreateFileW(path, (DWORD)access_mode, (DWORD)share_mode, NULL, (DWORD)create_mode, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) { throw std::invalid_argument("create file error"); }
	if (GetFileSizeEx(file, (PLARGE_INTEGER)&size) != TRUE) { throw std::runtime_error("get file size error"); }
	if (size > 0) { DoMapping(); }
}

FileManager::~FileManager() {
	UndoMapping();
	CloseHandle(file);
}

void FileManager::SetSize(uint64 size) {
	if (this->size != size) {
		UndoMapping();
		if (SetFilePointerEx(file, (LARGE_INTEGER&)size, NULL, FILE_BEGIN) != TRUE) { throw std::runtime_error("set file pointer error"); }
		if (SetEndOfFile(file) != TRUE) { throw std::runtime_error("set end of file error"); }
		this->size = size;
		DoMapping();
	}
}

void FileManager::DoMapping() {
	mapping = CreateFileMappingW(file, NULL, access_mode == AccessMode::ReadOnly ? PAGE_READONLY : PAGE_READWRITE, 0, 0, NULL);
	if (mapping == NULL) { throw std::runtime_error("create file mapping error"); }
}

void FileManager::UndoMapping() {
	Unlock();
	if (mapping != NULL) { CloseHandle(mapping); mapping = NULL; }
}

void FileManager::Unlock() {
	if (view_address != nullptr) { UnmapViewOfFile(view_address); view_address = nullptr; view_offset = 0; view_length = 0; }
}

byte* FileManager::Lock(uint64 offset, uint64 length) {
	if (mapping == NULL) { throw std::runtime_error("file mapping invalid"); }
	if (!Interval(0, size).Contains(Interval(offset, length))) { throw std::invalid_argument("invalid offset and length"); }
	if (Interval(view_offset, view_length).Contains(Interval(offset, length))) { return view_address + offset - view_offset; }
	Unlock();
	uint64 view_begin = align_offset_floor(offset, allocation_granularity);
	uint64 view_end = align_offset_ceil(offset + length, allocation_granularity); view_end = view_end <= size ? view_end : size;
	view_offset = view_begin; view_length = view_end - view_begin;
	view_address = (byte*)MapViewOfFile(mapping, access_mode == AccessMode::ReadOnly ? FILE_MAP_READ : (FILE_MAP_READ | FILE_MAP_WRITE), view_offset >> 32, (DWORD)view_offset, view_length);
	if (view_address == NULL) { throw std::runtime_error("map view of file error"); }
	return view_address + offset - view_offset;
}


END_NAMESPACE(BlockStore)