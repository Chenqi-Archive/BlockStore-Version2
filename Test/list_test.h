#include "BlockStore/file_manager.h"
#include "BlockStore/block_manager.h"
#include "BlockStore/stl_helper.h"

#include <iostream>


using namespace BlockStore;


struct Node {
	int number = 0;
	std::variant<BlockRef<Node>, nullptr_t> next = nullptr;
};

auto layout(layout_type<Node>) { return declare(&Node::number, &Node::next); }

using RootRef = BlockRef<Node>;


void PrintList(RootRef root) {
	BlockRef<Node> next = root;
	while (true) {
		auto node = next.Read();
		std::cout << node->number << std::endl;
		if (auto ptr = std::get_if<BlockRef<Node>>(&node->next); ptr) {
			next = *ptr;
		} else {
			break;
		}
	}
}


void AppendList(RootRef& root) {
	BlockRef<Node> prev = root;
	auto node = root.Write();
	node->number++;
	node->next = prev;
}


int main() {
	std::unique_ptr<FileManager> file;
	try {
		file.reset(new FileManager(L"R:\\list_test.dat"));
	} catch (std::runtime_error&) {
		return 0;
	}

	BlockManager manager(std::move(file));
	RootRef root;
	try {
		manager.LoadRootRef(root);
		PrintList(root);
		AppendList(root);
	} catch (std::exception&) {
		manager.Format();
		root = RootRef(manager);
	}
	manager.SaveRootRef(root);
}