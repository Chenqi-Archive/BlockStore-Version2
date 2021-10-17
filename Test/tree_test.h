#include "BlockStore/file_manager.h"
#include "BlockStore/block_loader.h"
#include "BlockStore/block_saver.h"
#include "BlockStore/stl/vector.h"

#include <iostream>


using namespace BlockStore;


struct TreeNode {
	std::string text;
	std::vector<BlockRef<TreeNode>> child_list;
};

constexpr auto layout(layout_type<TreeNode>) { return declare(&TreeNode::text, &TreeNode::child_list); }

using RootRef = BlockRef<TreeNode>;


void PrintNode(const TreeNode& node, uint depth) {
	static constexpr uint max_depth = 127;
	static const std::string tab_padding(max_depth, '\t');
	static const std::string_view tab_padding_view = tab_padding;
	static auto indent = [](uint level) { return tab_padding_view.substr(0, level); };

	std::cout << indent(depth) << node.text << std::endl;
	for (auto& node_ref : node.child_list) {
		auto node = node_ref.Load();
		PrintNode(*node, depth + 1);
	}
}

void PrintTree(RootRef& root) {
	auto node = root.Load();
	PrintNode(*node, 0);
}


void BuildTree(RootRef& root) {
	auto node = root.Create();
	node->text = "root block";
	node->child_list.resize(5, root.AsParent());
	size_t child_index = 0;
	for (auto& child_ref : node->child_list) {
		auto child_node = child_ref.Create();
		child_node->text = "child " + std::to_string(child_index++);
		child_node->child_list.resize(child_index, child_ref.AsParent());
		size_t child_index = 0;
		for (auto& child_ref : child_node->child_list) {
			auto child_node = child_ref.Create();
			child_node->text = "grand child " + std::to_string(child_index++);
		}
	}
}


int main() {
	FileManager file;
	try {
		file.Open(L"R:\\test.dat");
	} catch (std::runtime_error&) {
		return 0;
	}

	RootRef root;
	try {
		file.AsBlockLoader().LoadRootRef(root);
		PrintTree(root);
	} catch (std::runtime_error&) {
		file.Format();
		BuildTree(root);
		file.AsBlockSaver().SaveRootRef(root);
	}

	file.Close();
}