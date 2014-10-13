#include<iostream>
#include<cstdint>
#include<algorithm>
#include<array>
#include<vector>
#include<memory>

using namespace std;

typedef uint8_t Key [8];
typedef int Value;

const uint8_t EMPTY(50);

enum class Nodetype : uint8_t { SVLeaf = 1, Node4 = 4, Node16 = 16, Node48 = 48, Node256 = 5 };

struct BaseNode
{
	Nodetype type;
	uint8_t prefixLen;
	uint8_t prefix[8];
	BaseNode(Nodetype type) : type(type), prefixLen(0) {};
};
///	SVLeaf - Single-Value Leaf
struct SVLeaf : BaseNode
{
	Key key;
	unique_ptr<Value> value;
	SVLeaf() : BaseNode(Nodetype::SVLeaf) { this->value = nullptr; };
};

struct InnerNode : BaseNode
{
	int numChildren;
	InnerNode(Nodetype type) : BaseNode(type), numChildren(0) {};
};

struct Node4 : InnerNode
{
	array<uint8_t, 4> keys;
	array<shared_ptr<BaseNode>, 4> child;
	Node4() : InnerNode(Nodetype::Node4)
	{
		fill(child.begin(), child.end(), nullptr);
	}
};

struct Node16 : InnerNode
{
	array<uint8_t, 16> keys;
	array<shared_ptr<BaseNode>, 16> child;
	Node16() : InnerNode(Nodetype::Node16) 
	{
		fill(child.begin(), child.end(), nullptr);
		fill(keys.begin(), keys.end(), EMPTY);	
	}
};

struct Node48 : InnerNode
{
	array<uint8_t, 256> index;
	array<shared_ptr<BaseNode>, 48> child;
	Node48() : InnerNode(Nodetype::Node48)
	{
		fill(child.begin(), child.end(), nullptr);
		fill(index.begin(), index.end(), EMPTY);
	}
};

struct Node256 : InnerNode
{
	array<shared_ptr<BaseNode>, 256> child;
	Node256() : InnerNode(Nodetype::Node256)
	{
		fill(child.begin(), child.end(), nullptr);
	}
};
/// Tested
/* 
 * @brief: returns whether a node is a leaf or not
 * @params:	node 		node to check for being a leaf
 **/
bool inline isLeaf(shared_ptr<BaseNode> node)
{
	return (node->type == Nodetype::SVLeaf);
}
/// Tested
/* @brief: Find a child in an inner node given a partial key(byte)
 * @params:	node 		the node we search the child in
 *		byte		searched child corresponding partial key
 */
shared_ptr<BaseNode> findChild(shared_ptr<BaseNode> node, uint8_t byte)
{
	if(node->type == Nodetype::Node4)
	{
		shared_ptr<Node4> tmp = static_pointer_cast<Node4>(node);
		auto index = find(tmp->keys.begin(), tmp->keys.end(), byte);
		if(index != tmp->keys.end())
			return tmp->child[index-tmp->keys.begin()];
		return nullptr;
	}
	if(node->type == Nodetype::Node16)
	{
		shared_ptr<Node16> tmp = static_pointer_cast<Node16>(node);
		auto index = find(tmp->keys.begin(), tmp->keys.end(), byte);
		if(index != tmp->keys.end())
			return tmp->child[index-tmp->keys.begin()];
		return nullptr;
	}
	if(node->type == Nodetype::Node48)
	{
		shared_ptr<Node48> tmp = static_pointer_cast<Node48>(node);
		if(tmp->index[byte] != EMPTY)
			return tmp->child[tmp->index[byte]];
		return nullptr;
	}
	auto tmp = static_pointer_cast<Node256>(node);
	return tmp->child[byte];
}
/// Tested
/* check if leaf fully matches the key
 *
 */
bool leafMatches(shared_ptr<BaseNode> node, const Key& key, uint8_t depth)
{
	auto leaf = static_pointer_cast<SVLeaf>(node);
	return equal(leaf->key+depth, leaf->key+8, key+depth);	
}
/// Tested
/* @brief: returns number of bytes where key and node's prefix equal from certain depth
 *
 */
int checkPrefix(shared_ptr<BaseNode> node, const Key& key, uint8_t depth)
{
	auto count = 0;
	for(auto i = depth; (key[i] == node->prefix[i]) && (i < depth + node->prefixLen); i++)
		count++;
	return count;
}
/// Tested
/// searches for a key starting from node and returns key corresponding leaf
shared_ptr<BaseNode> search(shared_ptr<BaseNode> node, const Key& key, uint8_t depth)
{
cout << "Search" << endl;
	if(node == nullptr)
		return nullptr;
cout << "IsLeaf = " << (isLeaf(node)) << endl;
cout << "Depth = " << (int)depth << endl;
	if(isLeaf(node))
	{
		if(leafMatches(node, key, depth))
			return node;
		return nullptr;
	}
cout << (checkPrefix(node, key, depth) != node->prefixLen) << endl;
	if(checkPrefix(node, key, depth) != node->prefixLen)
		return nullptr;
cout << "prefixLen = " << (int)node->prefixLen << endl;
	depth += node->prefixLen;
	auto next = findChild(node, key[depth]);
cout << "next = " << next << endl;
	return search(next, key, depth+1);
} 
/// Tested - take care as script tells parent musn't be a SVLeaf and parent musn't be full
/// Appends a new child to an inner node NUMCHILDREN++
void addChild(shared_ptr<BaseNode>& parent, uint8_t byte, shared_ptr<BaseNode>& child)
{
	//Case for total beginning root of ART root = nullptr! ! !
	if(parent == nullptr)
	{
		parent = child;
		return;
	}
	if(parent->type == Nodetype::Node4)
	{
		auto tmp_parent = static_pointer_cast<Node4>(parent);
		tmp_parent->numChildren++;
		tmp_parent->keys[tmp_parent->numChildren-1] = byte;		
		sort(tmp_parent->keys.begin(), tmp_parent->keys.begin() + (tmp_parent->numChildren)-1);
		auto index_itr = find(tmp_parent->keys.begin(), tmp_parent->keys.end(), byte);
		auto index = index_itr - tmp_parent->keys.begin();
		move_backward(tmp_parent->child.begin() + index, tmp_parent->child.begin()+tmp_parent->numChildren-1, tmp_parent->child.begin() + index + 1);
		tmp_parent->child[index] = child;
for_each(tmp_parent->child.begin(), tmp_parent->child.end(), [](shared_ptr<BaseNode>& n){ cout << n << endl; }); 
		return;
	}
	if(parent->type == Nodetype::Node16)
	{
		auto tmp_parent = static_pointer_cast<Node16>(parent);
		tmp_parent->numChildren++;
		tmp_parent->keys[tmp_parent->numChildren-1] = byte;
		sort(tmp_parent->keys.begin(), tmp_parent->keys.begin() + (tmp_parent->numChildren)-1);
		auto index_itr = find(tmp_parent->keys.begin(), tmp_parent->keys.end(), byte);
		auto index = index_itr - tmp_parent->keys.begin();
		move_backward(tmp_parent->child.begin() + index, tmp_parent->child.begin()+tmp_parent->numChildren-1, tmp_parent->child.begin() + index + 1);
		tmp_parent->child[index] = child;
		return;
	}
	if(parent->type == Nodetype::Node48)
	{
		auto tmp_parent = static_pointer_cast<Node48>(parent);
		tmp_parent->child[tmp_parent->numChildren] = child;
		tmp_parent->index[byte] = tmp_parent->numChildren;		
		tmp_parent->numChildren++;	
		return;	
	}
	auto tmp_parent = static_pointer_cast<Node256>(parent);
	tmp_parent->child[byte] = child;
	tmp_parent->numChildren++;
}
/// Tested
/// Returns if a node is full 
bool isFull(const shared_ptr<BaseNode>& node)
{
	auto tmp_node = static_pointer_cast<InnerNode>(node);
	return (tmp_node->type == Nodetype::Node256) ? (tmp_node->numChildren == 256) : (tmp_node->numChildren == (int)tmp_node->type);
}
/// grows to one step bigger node e.g. Node4 -> Node16
void grow(shared_ptr<InnerNode>& node)
{
	if(node->type == Nodetype::Node4)
	{
		auto newNode = make_shared<Node16>();
		auto tmp_node = static_pointer_cast<Node4>(node);
		move(tmp_node->keys.begin(), tmp_node->keys.end(), newNode->keys.begin());
		move(tmp_node->child.begin(), tmp_node->child.end(), newNode->child.begin());
		newNode->numChildren = tmp_node->numChildren;
		newNode->prefixLen = tmp_node->prefixLen;
		copy(tmp_node->prefix, tmp_node->prefix + tmp_node->prefixLen, newNode->prefix);
		node = newNode;
		return; 
	}
	if(node->type == Nodetype::Node16)
	{
		shared_ptr<BaseNode> newNode = make_shared<Node48>();
		auto tmp_node = static_pointer_cast<Node16>(node);
		auto count = 0;
		for_each(tmp_node->keys.begin(), tmp_node->keys.end(), [&count, &newNode, &tmp_node](uint8_t key)mutable{ addChild(newNode, key, tmp_node->child[count]); count++; });
		newNode->prefixLen = tmp_node->prefixLen;
		copy(tmp_node->prefix, tmp_node->prefix + tmp_node->prefixLen, newNode->prefix);
		
		node = static_pointer_cast<InnerNode>(newNode);
		return;
	}
	if(node->type == Nodetype::Node48)
	{
		auto newNode = make_shared<Node256>();
		auto tmp_node = static_pointer_cast<Node48>(node);
		newNode->prefixLen = tmp_node->prefixLen;
		copy(tmp_node->prefix, tmp_node->prefix + tmp_node->prefixLen, newNode->prefix);
		for(unsigned int i = 0; i < tmp_node->index.size(); i++)
		{
			if(tmp_node->index[i] != EMPTY)
				newNode->child[i] = tmp_node->child[tmp_node->index[i]];
		}	
		node = newNode;
		return;
	}
}
/// TODO 
void insert(shared_ptr<BaseNode>& node, const Key& key, shared_ptr<BaseNode>& leaf, int depth)
{
	if(node == nullptr)
	{
		node = leaf;
		return;
	}
	if(isLeaf(node))
	{
		shared_ptr<BaseNode> newNode = make_shared<Node4>();
		auto tmp_leaf = static_pointer_cast<SVLeaf>(node);
		Key key2;
		copy(tmp_leaf->key, tmp_leaf->key+8, key2);
		auto i = depth;
		for(; (key[i] == key2[i]) && (i < 8); i++)
			newNode->prefix[i-depth] = key[i];
		newNode->prefixLen = i - depth;		
		depth += newNode->prefixLen;
		addChild(newNode, key[depth], leaf);
		addChild(newNode, key2[depth], node);	
		node = newNode;
		return;
	}
	auto p = checkPrefix(node, key, depth);
	if(p != node->prefixLen)
	{
		shared_ptr<BaseNode> newNode = make_shared<Node4>();
		addChild(newNode, key[depth+p], leaf);
		addChild(newNode, node->prefix[p], node);
		newNode->prefixLen = p;
		copy(node->prefix, node->prefix + node->prefixLen, newNode->prefix);
		node->prefixLen = node->prefixLen-(p+1);
		copy(node->prefix, node->prefix + node->prefixLen, node->prefix + (p+1));
		node = newNode;
		return;
	}
	depth += node->prefixLen;
	auto next = findChild(node, key[depth]);
	if(next != nullptr)
		insert(next, key, leaf, depth+1);
	else
	{
		//non-const reference passed ! ! ! beautify this part ! ! 
		shared_ptr<InnerNode> tmp_node = static_pointer_cast<InnerNode>(node);
		if(isFull(tmp_node))
			grow(tmp_node);
		shared_ptr<BaseNode> tmp_node2 = static_pointer_cast<InnerNode>(tmp_node);
		addChild(tmp_node2, key[depth], leaf);
		node = tmp_node2;
	}
}

int main()
{
	shared_ptr<BaseNode> root = nullptr;
	
	shared_ptr<SVLeaf> leaf = make_shared<SVLeaf>();
	shared_ptr<SVLeaf> leaf2 = make_shared<SVLeaf>();
	shared_ptr<SVLeaf> leaf3 = make_shared<SVLeaf>();
	shared_ptr<SVLeaf> leaf4 = make_shared<SVLeaf>();
	shared_ptr<SVLeaf> leaf5 = make_shared<SVLeaf>();

	Key test_key = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	Key test_key2 = { 0x00, 0x06, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08 };
	Key test_key3 = { 0x00, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	Key test_key4 = { 0x00, 0x04, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	Key test_key5 = { 0x00, 0x03, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };

	
	copy(test_key, test_key+8, leaf->key);
	copy(test_key2, test_key2+8, leaf2->key);
	copy(test_key3, test_key3+8, leaf3->key);
	copy(test_key4, test_key4+8, leaf4->key);
	copy(test_key5, test_key5+8, leaf5->key);

	auto tmp_leaf = static_pointer_cast<BaseNode>(leaf);	
	auto tmp_leaf2 = static_pointer_cast<BaseNode>(leaf2);
	auto tmp_leaf3 = static_pointer_cast<BaseNode>(leaf3);	
	auto tmp_leaf4 = static_pointer_cast<BaseNode>(leaf4);	
	auto tmp_leaf5 = static_pointer_cast<BaseNode>(leaf5);	

	insert(root, test_key, tmp_leaf, 0);
	insert(root, test_key2, tmp_leaf2, 0);
	insert(root, test_key3, tmp_leaf3, 0);
	insert(root, test_key4, tmp_leaf4, 0);
	cout << "Insert leaf = " << leaf << endl;
	cout << "Insert leaf2 = " << leaf2 << endl;
	cout << "Insert leaf3 = " << leaf3 << endl;
	cout << "Insert leaf4 = " << leaf4 << endl;

	cout << "Insert leaf5 = " << leaf5 << endl;
	insert(root, test_key5, tmp_leaf5, 0);


cout << endl;
auto tmp_root = static_pointer_cast<Node16>(root);
cout<<"keys"<<endl;	
	for_each(tmp_root->keys.begin(), tmp_root->keys.end(), [](uint8_t key){ cout << (int)key << endl; });
cout<<"child"<<endl;
	for_each(tmp_root->child.begin(), tmp_root->child.end(), [](shared_ptr<BaseNode>& n){ cout << n << endl; }); 

	cout << "Found leaf at " << search(root, test_key, 0) << endl;
	cout << "Found leaf2 at " << search(root, test_key2, 0) << endl;
	cout << "Found leaf3 at " << search(root, test_key3, 0) << endl;
	cout << "Found leaf4 at " << search(root, test_key4, 0) << endl;
	cout << "Found leaf5 at " << search(root, test_key5, 0) << endl;

//Build nodes
/*	shared_ptr<Node4> n0 = make_shared<Node4>();
	shared_ptr<Node16> n1 = make_shared<Node16>();
	shared_ptr<Node48> n2 = make_shared<Node48>();
	shared_ptr<Node256> n3 = make_shared<Node256>();
	shared_ptr<Node4> n4 = make_shared<Node4>();
	shared_ptr<Node4> n5 = make_shared<Node4>();
	shared_ptr<Node4> n6 = make_shared<Node4>();
	shared_ptr<Node4> n7 = make_shared<Node4>();
	shared_ptr<SVLeaf> leaf = make_shared<SVLeaf>();
	shared_ptr<SVLeaf> leaf2 = make_shared<SVLeaf>();
//set leaf
	leaf->key[0] = 0x00;
	leaf->key[1] = 0x01;
	leaf->key[2] = 0x02;
	leaf->key[3] = 0x03;
	leaf->key[4] = 0x04;
	leaf->key[5] = 0x05;
	leaf->key[6] = 0x06;
	leaf->key[7] = 0x07;
//set leaf2
	leaf2->key[0] = 0x00;
	leaf2->key[1] = 0x01;
	leaf2->key[2] = 0x02;
	leaf2->key[3] = 0x03;
	leaf2->key[4] = 0x04;
	leaf2->key[5] = 0x05;
	leaf2->key[6] = 0x06;
	leaf2->key[7] = 0x08;
	
	n0->prefix[0] = 0x00;
	n0->prefix[1] = 0x01;
	n0->prefix[2] = 0x02;
	n0->prefix[3] = 0x03;
	n0->prefix[4] = 0x04;
	n0->prefix[5] = 0x05;
	n0->prefix[6] = 0x06;
	
	n0->prefixLen = 7;
//chain nodes
	n0->keys[0] = 0x07;
	n0->keys[1] = 0x08;
	n0->child[0] = leaf;
	n0->child[1] = leaf2;
	n1->keys[0] = 0x01;
	n2->index[0x02] = 0;
	n4->keys[0] = 0x04;
	n5->keys[0] = 0x05;
	n6->keys[0] = 0x06;
	n7->keys[0] = 0x07;
/	
// 
//	n0->child[0] = n1;
//	n1->child[0] = n2;
	n2->child[0] = n3;
	n3->child[0x03] = n4;
	n4->child[0] = n5;
	n5->child[0] = n6;
	n6->child[0] = n7;
	n7->child[0] = leaf;

	Key test_key = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
*/
	
	
	return 0;
}
