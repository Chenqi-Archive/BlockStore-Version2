#include "BlockStore/file_manager.h"
#include "BlockStore/block_manager.h"

#include <iostream>


using namespace BlockStore;


struct Node {
	int number = 0;
	BlockRef<Node> next;
};

auto layout(layout_type<Node>) { return declare(&Node::number, &Node::next); }

using RootRef = BlockRef<Node>;


void PrintList(RootRef root) {
	BlockRef<Node> next = root;
	while (!next.IsEmpty()) {
		auto node = next.Read();
		std::cout << node->number << std::endl;
		next = node->next;
	}
}


void AppendList(RootRef& root) {
	BlockRef<Node> prev = root;
	auto& node = root.Write();
	node.number++;
	node.next = prev;
}


int main() {
	std::unique_ptr<FileManager> file;
	try {
		file.reset(new FileManager(L"R:\\list_test.dat"));
	} catch (std::runtime_error&) {
		return 0;
	}

	BlockManager manager(std::move(file));
	RootRef root; manager.LoadRootRef(root);
	try {
		PrintList(root);
	} catch (std::exception&) {
		manager.Format();
		manager.LoadRootRef(root);
	}
	AppendList(root);
	manager.SaveRootRef(root);
}