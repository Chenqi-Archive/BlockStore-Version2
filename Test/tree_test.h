#include "BlockStore/file_manager.h"
#include "BlockStore/block_loader.h"

#include <iostream>


using namespace BlockStore;


struct TreeNode {
	std::string text;
	std::vector<BlockRef<TreeNode>> child_list;
};

constexpr auto layout(layout_type<TreeNode>) { return declare(&TreeNode::text, &TreeNode::child_list); }


using Root = BlockRef<TreeNode>;


void PrintNode(TreeNode& node, uint depth) {
	static constexpr uint max_depth = 127;
	static const std::string tab_padding(max_depth, '\t');
	static const std::string_view tab_padding_view = tab_padding;
	static auto indent = [](uint level) { return tab_padding_view.substr(0, level); };

	std::cout << indent(depth) << node.text << std::endl;
	for (auto& node_ref : node.child_list) {
		PrintNode(*node_ref, depth + 1);
	}
}

void PrintTree(Root& root) {
	PrintNode(*root, 0);
}


void BuildTree(Root& root) {
	TreeNode& node = *root;
	node.text = "some test string";
	node.child_list.resize(15);
	size_t child_index = 0;
	for (auto& child : node.child_list) {
		(*child).text = "child " + std::to_string(child_index);
	}
}


int main() {
	Root root;
	BuildTree(root);
	FileManager file(L"R:\\test.dat");


	Root root(file, 0);
	PrintTree(root);
}