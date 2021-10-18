#include "BlockStore/file_manager.h"
#include "BlockStore/block_manager.h"
#include "BlockStore/stl_helper.h"

#include <iostream>


using namespace BlockStore;


struct TreeNode {
	std::string text;
	std::vector<BlockRef<TreeNode>> child_list;
};

constexpr auto layout(layout_type<TreeNode>) { return declare(&TreeNode::text, &TreeNode::child_list); }

using RootRef = BlockRef<TreeNode>;


void PrintTree(BlockRef<TreeNode> node_ref, uint depth = 0) {
	static constexpr uint max_depth = 127;
	static const std::string tab_padding(max_depth, '\t');
	static const std::string_view tab_padding_view = tab_padding;
	static auto indent = [](uint level) { return tab_padding_view.substr(0, level); };

	auto node = node_ref.Read();
	std::cout << indent(depth) << node->text << std::endl;
	for (auto& node_ref : node->child_list) {
		PrintTree(node_ref, depth + 1);
	}
}


void BuildTree(RootRef& root) {
	auto& node = root.Write();
	node.text = "root block";
	node.child_list.resize(5, root.GetManager());
	data_t child_index = 0;
	for (auto& child_ref : node.child_list) {
		auto& child_node = child_ref.Write();
		child_node.text = "child " + std::to_string(child_index++);
		child_node.child_list.resize(child_index, child_ref.GetManager());
		data_t child_index = 0;
		for (auto& child_ref : child_node.child_list) {
			auto& child_node = child_ref.Write();
			child_node.text = "grand child " + std::to_string(child_index++);
		}
	}
}


int main() {
	std::unique_ptr<FileManager> file;
	try {
		file.reset(new FileManager(L"R:\\test.dat"));
	} catch (std::runtime_error&) {
		return 0;
	}

	BlockManager manager(std::move(file));
	RootRef root; manager.LoadRootRef(root);
	try {
		PrintTree(root);
	} catch (std::runtime_error&) {
		manager.Format();
		BuildTree(root);
		manager.SaveRootRef(root);
	}
}