#include<iostream>
#include<cstdint>
#include<algorithm>
#include<array>
#include<vector>
#include<memory>

using namespace std;

typedef uint8_t Key [8];
typedef int Value;

const uint8_t EMPTY(-1);

enum class Nodetype : uint8_t { SVLeaf = 1, Node4 = 2, Node16 = 3, Node48 = 4, Node256 = 5 };

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

struct Node : BaseNode
{
	int numChildren;
	Node(Nodetype type) : BaseNode(type), numChildren(0) {};
};

struct Node4 : Node
{
	array<uint8_t, 4> keys;
	array<shared_ptr<BaseNode>, 4> child;
	Node4() : Node(Nodetype::Node4)
	{
		fill(child.begin(), child.end(), nullptr);
	}
};

struct Node16 : Node
{
	array<uint8_t, 16> keys;
	array<shared_ptr<BaseNode>, 16> child;
	Node16() : Node(Nodetype::Node16) 
	{
		fill(child.begin(), child.end(), nullptr);	
	}
};

struct Node48 : Node
{
	array<uint8_t, 256> index;
	array<shared_ptr<BaseNode>, 48> child;
	Node48() : Node(Nodetype::Node48)
	{
		fill(child.begin(), child.end(), nullptr);
		fill(index.begin(), index.end(), EMPTY);
	}
};

struct Node256 : Node
{
	array<shared_ptr<BaseNode>, 256> child;
	Node256() : Node(Nodetype::Node256)
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
	if(node == nullptr)
		return nullptr;
	if(isLeaf(node))
	{
		if(leafMatches(node, key, depth))
			return node;
		return nullptr;
	}
	if(checkPrefix(node, key, depth) != node->prefixLen)
		return nullptr;
	depth += node->prefixLen;
	auto next = findChild(node, key[depth]);
	return search(next, key, depth+1);
} 
/// Tested - take care as script tells parent musn't be a SVLeaf and parent musn't be full
/// Appends a new child to an inner node NUMCHILDREN++
void addChild(shared_ptr<BaseNode>& parent, uint8_t byte, shared_ptr<BaseNode>& child)
{
	//Case for total beginning root of ART = nullptr! ! !
	if(parent == nullptr)
	{
		parent = child;
		return;
	}
	if(parent->type == Nodetype::Node4)
	{
		auto tmp_parent = static_pointer_cast<Node4>(parent);
		tmp_parent->keys[tmp_parent->numChildren] = byte;
		tmp_parent->numChildren++;
		sort(tmp_parent->keys.begin(), tmp_parent->keys.begin() + (tmp_parent->numChildren));
		auto index_itr = find(tmp_parent->keys.begin(), tmp_parent->keys.end(), byte);
		auto index = index_itr - tmp_parent->keys.begin();
		copy(tmp_parent->child.begin() + index, tmp_parent->child.begin() + tmp_parent->numChildren, tmp_parent->child.begin() + index + 1);
		tmp_parent->child[index] = child;

		return;
	}
	if(parent->type == Nodetype::Node16)
	{
		auto tmp_parent = static_pointer_cast<Node16>(parent);
		tmp_parent->keys[tmp_parent->numChildren] = byte;
		tmp_parent->numChildren++;
		sort(tmp_parent->keys.begin(), tmp_parent->keys.begin() + (tmp_parent->numChildren));
		auto index_itr = find(tmp_parent->keys.begin(), tmp_parent->keys.end(), byte);
		auto index = index_itr - tmp_parent->keys.begin();
		copy(tmp_parent->child.begin() + index, tmp_parent->child.begin() + tmp_parent->numChildren, tmp_parent->child.begin() + index + 1);
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

int main()
{
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
	shared_ptr<BaseNode> root =  make_shared<Node256>(); //why no auto ?
	shared_ptr<BaseNode> leaf = make_shared<SVLeaf>();
	cout << "Leaf = " << leaf << endl;
	shared_ptr<BaseNode> leaf2 = make_shared<SVLeaf>();
	cout << "Leaf2 = " << leaf2 << endl;
	addChild(root, 0x01, leaf);
	addChild(root, 0x00, leaf2);
	cout << "Found leaf at = " << findChild(root, 0x01) << endl;
	cout << "Found leaf2 at = " << findChild(root, 0x00) << endl;

	return 0;
}
