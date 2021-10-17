#include "BlockStore/file_manager.h"


using namespace BlockStore;


int main() {
	FileManager file(L"R:\\test");
	file.SetSize(700);
	byte* data = file.Lock(200, 500);
	int a = -1;
	memcpy(data, &a, 4);
}