#include "BlockStore/file_manager.h"
#include "BlockStore/block_manager.h"

#include <iostream>


using namespace BlockStore;


struct Node {
	int number;
	BlockRef<Node> next;
};

auto layout(layout_type<Node>) { return declare(&Node::number, &Node::next); }

using RootRef = BlockRef<Node>;


void PrintRing(RootRef root) {
	BlockRef<Node> next = root;
	do {
		auto node = next.Read();
		std::cout << node->number << std::endl;
		next = node->next;
	} while (next != root);
}


void BuildRing(RootRef& root, uint number) {
	if (number <= 1) {
		auto node = root.Write();
		node->number = number;
		node->next = root;
	} else {
		BlockRef<Node> last = root.GetManager();
		last.Write()->number = number;
		BlockRef<Node> prev = last;
		BlockRef<Node> curr = root.GetManager();
		while (--number) {
			auto node = curr.Write();
			node->number = number;
			node->next = prev;
			prev = curr;
			curr = BlockRef<Node>(root.GetManager());
		}
		root = prev;
		last.Write()->next = root;
	}
}


int main() {
	std::unique_ptr<FileManager> file;
	try {
		file.reset(new FileManager(L"R:\\ring_test.dat"));
	} catch (std::runtime_error&) {
		return 0;
	}

	BlockManager manager(std::move(file));
	RootRef root;
	try {
		manager.LoadRootRef(root);
		PrintRing(root);
	} catch (std::exception&) {
		manager.Format();
		root = RootRef(manager);
	}
	BuildRing(root, 5);
	manager.SaveRootRef(root);
}