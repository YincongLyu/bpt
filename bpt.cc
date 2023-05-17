#include "bpt.h"
#include <stdlib.h>
#include <assert.h>

#include <algorithm>



namespace bpt {

OPERATOR_KEYCMP(index_t)
OPERATOR_KEYCMP(record_t)

/**
 * 构造函数, 顺便初始化fp和fp_level
 * 入参：文件路径 + 可指定是否强制为空树
*/
bplus_tree::bplus_tree(const char *p, bool force_empty) : fp(NULL), fp_level(0) {
    // 清零，并对成员path赋值外界传入的文件路径
    bzero(path, sizeof(path));
    strcpy(path, p);

    // 这里加强了判断，如果外部虽然不强制为空，但path路径给的是错误的，需要手动纠正
    if (!force_empty) {
        FILE *fp = fopen(path, "r");
        if (!fp) 
            force_empty = true;
        else 
            fclose(fp);
    }
    if (force_empty)
        init_from_empty();
    else
        map(&meta);
}

/**
 * 构建空树: 初始化meta数据+root节点
*/
void bplus_tree::init_from_empty() {
    // init default meta data
    bzero(&meta, sizeof(meta_t));
    meta.order = BP_ORDER;
    // 预分配128个内部节点
    meta.index_size = sizeof(internal_node_t) * 128;
    meta.value_size = sizeof(value_t);
    meta.key_size = sizeof(key_t);
    meta.height = 1;
    meta.index_slot = 0;
    meta.block_slot = 0;
    // init root node
    internal_node_t root;
    meta.root_offset = alloc(&root); 
    // init empty leaf
    leaf_node_t leaf; // 建立双向关系
    root.children[0].child = alloc(&leaf); // leaf is the first child of root
    leaf.parent = meta.root_offset; // leaf's parent is root
    unmap(&meta);
    unmap(&root, meta.root_offset);
    unmap(&leaf, root.children[0].child);
}


// 定位node's offset  according to key
off_t bplus_tree::search_index(const key_t &key) const {
    off_t org = meta.root_offset;
    int height = meta.height;
    while (height > 1) {
        internal_node_t node;
        map(&node, org);
        index_t *idx = std::upper_bound(node.children, node.children + node.n, key);
        org = idx->child;
        height--;  // 这里看起来每层只有一个internal node，indeed，每次都向下一层
    }
    return org;
}


// 进一步调用 search_index
off_t bplus_tree::search_leaf(const key_t &key) const {
    internal_node_t node;
    map(&node, search_index(key));

    index_t *idx = std::upper_bound(node.children, node.children + node.n, key);
    return idx->child;
}


//当前test只关心 function是否执行成功
int bplus_tree::search(const key_t &key, value_t *value) const {
    leaf_node_t leaf;
    // map_block(&leaf, search_leaf_offset(key));
    map(&leaf, search_leaf(key));
    // 根据key找到record
    record_t *record = std::lower_bound(leaf.children, leaf.children + leaf.n, key);
    if (record != leaf.children + leaf.n) {
        *value = record->value; // find the record, return the value
        return keycmp(record->key, key);
    } else {
        return -1;
    }
}


int bplus_tree::insert(const key_t &key, const value_t &value) {
    leaf_node_t leaf;
    // map_block(&leaf, search_leaf_offset(key));
    off_t offset = search_leaf(key);
    map(&leaf, offset);
    
    if (leaf.n == meta.order) {
        // 插入的情况正好满了，需split
        // new sibling leaf
        leaf_node_t new_leaf;
        leaf.next = alloc(&new_leaf);

        // find even split point 决定key分到左孩子还是右孩子
        size_t point = leaf.n / 2;
        if (keycmp(key, leaf.children[point].key) > 0) {
            point++;
        }
        // split
        std::copy(leaf.children + point, leaf.children + leaf.n, new_leaf.children);
        new_leaf.n = leaf.n - point;
        leaf.n = point;

        // 节点分裂完insert key, which part to insert?
        if (keycmp(key, new_leaf.children[0].key) < 0) { // 小的插左边
            insert_leaf_no_split(&leaf, key, value);
        } else {
            insert_leaf_no_split(&new_leaf, key, value);
        }
        //把右孩子的第一个当成新的key作为internal_node的索引
        new_leaf.parent = leaf.parent = insert_key_to_index(
            leaf.parent, new_leaf.children[0].key, offset, leaf.next);
        unmap(&meta);
        unmap(&leaf, offset);
        unmap(&new_leaf, leaf.next);//insert_key_to_index()写入的leaf.next

    } else {
        insert_leaf_no_split(&leaf, key, value);
        unmap(&leaf, offset);
    }
    return 0;
}
void bplus_tree::insert_leaf_no_split(leaf_node_t *leaf, const key_t &key, const value_t &value) {
    // insert into array when leaf is not full
    record_t *r = std::upper_bound(leaf->children, leaf->children + leaf->n, key);
    //same key也插入

    std::copy(r, leaf->children + leaf->n, r + 1);
    
    r->key = key;
    r->value = value;
    leaf->n++;
}

int bplus_tree::insert_key_to_index(int offset, key_t key, off_t old, off_t after) {
    assert(offset >= -1);

    if (offset == -1) {
        // create new root node
        internal_node_t root;
        meta.root_offset = alloc(&root);
        meta.height++;
        // insert 'old' and 'after'
        root.n = 1;
        root.children[0].key = key;
        root.children[0].child = old;
        root.children[1].child = after;
        unmap(&meta);
        unmap(&root, meta.root_offset);
        return meta.root_offset;
    }

    internal_node_t node;
    map(&node, offset);
    assert(node.n + 1 <= meta.order);
    if (node.n + 1 == meta.order) {
        // split when full
        internal_node_t new_node;
        off_t new_node_offset = alloc(&new_node);
        //split
        //there are 'node.n + 1' elements in node.children
        size_t point = node.n / 2 + 1;
        std::copy(node.children + point, node.children + node.n + 1, new_node.children);
        new_node.n = node.n - point;
        node.n -= point + 1;

        // give the middle key to parent
        // middle key's child is reserved in node.children[node.n]
        new_node.parent = node.parent = insert_key_to_index(node.parent, node.children[point].key, offset, new_node_offset); // 递归调用
        unmap(&meta);
        unmap(&node, offset);
        unmap(&new_node, new_node_offset);
    } else {
        insert_key_to_index_no_split(&node, key, after);
        unmap(&node, offset);
    }
    return offset;
}

void bplus_tree::insert_key_to_index_no_split(internal_node_t *node, const key_t &key, off_t value) {
    index_t *idx = std::upper_bound(node->children, node->children + node->n, key);
    if (idx == node->children + node->n) {
        // add index key to end
        node->children[node->n].key = key;
        node->children[node->n + 1].child = value;
    } else {
        std::copy(idx, node->children + node->n + 1, idx + 1);
        idx->key = key;
        idx->child = value;
    }
    node->n++;
}


}
