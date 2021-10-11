#include "BlockStore/FileManager.h"
#include "BlockStore/block_ref.h"

#include <string>
#include <vector>
#include <iostream>


using namespace BlockStore;


struct TreeNode {
	std::string text;
	std::vector<BlockRef<TreeNode>> child_list;
};


using Root = BlockRef<TreeNode>;


BEGIN_NAMESPACE(BlockStore)

template<> 
constexpr auto layout<TreeNode>() {
	return declare(
		&TreeNode::text,
		&TreeNode::child_list
	);
}

END_NAMESPACE(BlockStore)


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


int main() {
	BlockManagerFile file(L"");
	Root root = file.LoadRoot<Root>();
	TreeNode& root_node = *root;
	PrintNode(root_node, 0);
}