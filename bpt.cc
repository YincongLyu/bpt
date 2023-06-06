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

    if (force_empty) {
        // create empty tree if file doesn't exist
        open_file("w+"); //truncate file: consider the path_name has already used
        init_from_empty();
        close_file();
    }
    else {
        // read tree from file
        if (map(&meta, OFFSET_META) != 0) {
            force_empty = true;
        }
    }
        
}

/**
 * 构建空树: 初始化meta数据+root节点
*/
void bplus_tree::init_from_empty() {
    // init default meta data
    bzero(&meta, sizeof(meta_t));
    meta.order = BP_ORDER;
   
    meta.value_size = sizeof(value_t);
    meta.key_size = sizeof(key_t);
    meta.height = 1;
    meta.slot = OFFSET_BLOCK;
    // init root node
    internal_node_t root;
    meta.root_offset = alloc(&root); 
    // init empty leaf
    leaf_node_t leaf; // 建立双向关系
    leaf.next = 0;
    meta.leaf_offset = root.children[0].child = alloc(&leaf); // leaf is the first child of root
    //save
    unmap(&meta, OFFSET_META);
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
off_t bplus_tree::search_leaf(off_t index, const key_t &key) const {
    internal_node_t node;
    map(&node, index);

    index_t *idx = std::upper_bound(node.children, node.children + node.n, key);
    return idx->child;
}


//当前test只关心 function是否执行成功
int bplus_tree::search(const key_t &key, value_t *value) const {
    leaf_node_t leaf;
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

int bplus_tree::search_range(key_t *left, const key_t &right, value_t *values, size_t max, key_t *next) const {

    if (left == NULL || keycmp(*left, right) > 0) {
        return -1;
    }
    off_t off_left = search_leaf(*left);
    off_t off_right = search_leaf(right);

    off_t off = off_left; // 第一个leaf的off是从off_left开始的
    size_t i = 0;
    record_t *begin, *end;
    leaf_node_t leaf;

    while (off != off_right && off != 0 && i < max) {
        map(&leaf, off);
        //start point
        if (off_left == off)
            begin = std::lower_bound(leaf.children, leaf.children + leaf.n, *left);
        else
            begin = leaf.children;

        //copy
        end = leaf.children + leaf.n;
        for (; begin != end && i < max; begin++, i++) {
            values[i] = begin->value;
        }
        off = leaf.next;
    }
    // the last leaf, this is a corner case using while loop parameter
    if (i < max) {
        map(&leaf, off_right); // 左闭右开
        begin = std::lower_bound(leaf.children, leaf.children + leaf.n, *left); 
        end = std::upper_bound(leaf.children, leaf.children + leaf.n, right);
        for (; begin != end && i < max; begin++, i++) { // I suspect 是否会重复插入？当off_right不是一个leaf的offset头
            values[i] = begin->value;
        }
    }
    // if (last != NULL && i == max && begin != end) {
    //     *last = begin->key;
    // }
    if (next != NULL) {
        if (i == max && begin != end) {
            *next = true;
            *left = begin->key;
        } else {
            *next = false;
        }
    }
    return i;
}

int bplus_tree::insert(const key_t &key, const value_t &value) {
    // leaf_node_t leaf;
    // off_t offset = search_leaf(key);
    // map(&leaf, offset);

    off_t parent = search_index(key); // 下面的search_leaf本身就会再次调用search_index，为什么这次修改要抽出来写？
    off_t offset = search_leaf(parent, key); // 那干脆不要去掉leaf_node的parent属性好了
    leaf_node_t leaf;
    map(&leaf, offset);
    
    if (leaf.n == meta.order) {
        // split when full
        // new sibling leaf
        leaf_node_t new_leaf;
        new_leaf.next = leaf.next; // new leaf is followed tightly behind the leaf
        leaf.next = alloc(&new_leaf);

        // find even split point 决定key分到左孩子还是右孩子
        size_t point = leaf.n / 2;
        bool place_right = keycmp(key, leaf.children[point].key) > 0;
        if (place_right) {
            point++;
        }
        // 完成原始leaf的split
        std::copy(leaf.children + point, leaf.children + leaf.n, new_leaf.children);
        new_leaf.n = leaf.n - point;
        leaf.n = point;

        // 节点分裂完insert key, which part to insert?
        if (place_right) { 
            insert_leaf_no_split(&new_leaf, key, value);      
        } else {
            insert_leaf_no_split(&leaf, key, value);
        }
        
        // save leafs
        unmap(&meta, OFFSET_META);
        unmap(&leaf, offset);
        unmap(&new_leaf, leaf.next);//insert_key_to_index()写入的leaf.next

        //把右孩子的第一个当成新的key作为internal_node的索引
        // new_leaf.parent = leaf.parent = insert_key_to_index(
        //     leaf.parent, new_leaf.children[0].key, offset, leaf.next);
        insert_key_to_index(parent, new_leaf.children[0].key, offset, leaf.next, true);

    } else {
        insert_leaf_no_split(&leaf, key, value);
        unmap(&leaf, offset);
    }
    return 0;
}
/**
 * 通过key定位出offset，找出对应的leaf
 * 在该leaf中二分查找该key的位置，可能是>=的情况，只有相等才能修改
*/
int bplus_tree::update(const key_t &key, value_t value) {
    off_t offset = search_leaf(key);
    leaf_node_t leaf;
    map(&leaf, offset);

    record_t *record = std::lower_bound(leaf.children, leaf.children + leaf.children.n, key);
    if (record != leaf.children + leaf.n) {
        if (keycmp(key, record->key) == 0) {
            record->value = value;
            unmap(&leaf, offset);
            return 0;
        } else {
            return 1;
        }
    } else {
        return -1;
    }
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

void bplus_tree::insert_key_to_index(off_t offset, const key_t &key, off_t old, off_t after, bool is_leaf) {

    if (offset == 0) { // init a leaf by noting parent with 0
        // create new root node
        internal_node_t root;
        meta.root_offset = alloc(&root);
        meta.height++;
        // insert 'old' and 'after'
        root.n = 1;
        root.children[0].key = key;
        root.children[0].child = old;
        root.children[1].child = after;
        unmap(&meta, OFFSET_META);
        unmap(&root, meta.root_offset);
        
        // update children's parent
        if (!is_leaf) {
            reset_index_children_parent(root.children, root.children + root.n + 1, meta.root_offset);
        }
        return;
    }

    internal_node_t node;
    map(&node, offset);
    assert(node.n + 1 <= meta.order);
    if (node.n + 1 == meta.order) {
        // split when full
        internal_node_t new_node;
        off_t new_node_offset = alloc(&new_node);
        
        
        // find even split point
        size_t point = node.n / 2;
        bool place_right = keycmp(key, node.children[point].key) > 0;
        if (place_right) {
            point++;
        }

        // prevent key being the right `middle_key`
        // eg：insert 48 into | 42 | 45 | 6 | |
        if (place_right && keycmp(key, node.children[point].key) < 0)
            point--;
        key_t middle_key = node.children[point].key;

        // split
        // there are 'node.n + 1' elements in node.children
        std::copy(node.children + point + 1, node.children + node.n + 1, new_node.children);
        new_node.n = node.n - point - 1;
        new_node.parent = node.parent;
        node.n = point;

        // put the new key
        if (place_right) 
            insert_key_to_index_no_split(&new_node, key, after);
        else
            insert_key_to_index_no_split(&node, key, after);
        unmap(&meta, OFFSET_META);
        unmap(&node, offset);
        unmap(&new_node, new_node_offset);
        // update children's parent
        if (!is_leaf) {
            reset_index_children_parent(new_node.children, new_node.children + new_node.n + 1, new_node_offset);
        }
        

        // give the middle key to parent
        // middle key's child is reserved in node.children[node.n]
        insert_key_to_index(node.parent, middle_key, offset, new_node_offset, false); // 递归调用
        
  
    } else {
        insert_key_to_index_no_split(&node, key, after);
        unmap(&node, offset);
    }
}
void bplus_tree::reset_index_children_parent(index_t *begin, index_t *end, off_t parent) {
    internal_node_t node;
    while (begin != end) {
        map(&node, begin->child);
        node.parent = parent;
        unmap(&node, begin->child);
        begin++;
    }
}

void bplus_tree::insert_key_to_index_no_split(internal_node_t *node, const key_t &key, off_t value) {
    index_t *idx = std::upper_bound(node->children, node->children + node->n, key);

    std::copy(idx, node->children + node->n + 1, idx + 1);

    // insert this key
    idx->key = key;
    idx->child = (idx + 1)->child; // I think 这里是有问题的，why 要交换这两个node指向的孩子？
    (idx + 1)->child = value; // 我认为经过深copy后idx+1的child也已经copy过去了，直接赋值idx处的key和value就行
    
    node->n++;
}

}
