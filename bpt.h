#ifndef BPT_H
#define BPT_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "predefined.h"


namespace bpt {

// offsets
#define OFFSET_META 0
#define OFFSET_BLOCK OFFSET_META + sizeof(meta_t)

//B+ tree的元数据
struct meta_t {
    size_t order; //B+ tree的顺序

    size_t value_size; // value的大小
    size_t key_size; // key的大小
    size_t internal_node_num; // 内部所有节点的数量
    size_t leaf_node_num; // 叶子节点的刷零
    size_t height; // 包括叶子节点那层的树高,root's height = 1
    off_t slot; // where to store new block, is a location presented by offset, differ from 

    off_t root_offset; // where is the root of internal nodes;
};

// 内部节点的index segment
struct index_t {
    key_t key;
    off_t child; // the offset of index in one internal node
};

// 内部节点的block组织形式，一个block指向下面的孩子节点
/**
 * internal node block
 * | int parent | size_t n | key_t key, int child | ... |
*/
struct internal_node_t {
    off_t parent; //额外记录父节点的offset
    size_t n; // 有多少孩子节点
    index_t children[BP_ORDER];
    internal_node_t() {}
};

// 这里的record仅是一条简单的 k/v，key当主键
struct record_t {
    key_t key;
    value_t value;
};

/**
 * leaf node block
 * | int prev | int next | size_t n | size_t i, key_t key, value_t value | ... |
*/
struct leaf_node_t {
    // int parent; // parent node offset, why don't continue to use prev?
    off_t next; // next leaf
    size_t n; // 当前leaf block所有的record数量
    record_t children[BP_ORDER];
    leaf_node_t() {}
};

class bplus_tree {
public:
    // 构造函数
    bplus_tree(const char *path, bool force_empty = false);

    //abstract function CRUD
    int search(const key_t &key, value_t *value) const;
    int erase(const key_t &key);
    int insert(const key_t &key, const value_t &value);
    int update(const key_t &key, const value_t &value);

#ifndef UNIT_TEST
private:
#else
public:
#endif
    char path[512];//文件读写路径

    meta_t meta;

    // 构建一个空树
    void init_from_empty();

    // find index offset
    off_t search_index(const key_t &key) const;
    // find leaf offset
    // 函数search leaf灵活入参 
    off_t search_leaf(off_t index, const key_t &key) const;
    off_t search_leaf(const key_t &key) const {
        return search_leaf(search_index(key), key);
    }


    
    // 以下三个函数 为节点做split用
    // insert into leaf without split
    void insert_leaf_no_split(leaf_node_t *leaf, const key_t &key, const value_t &value);
    // add key to the internal node
    void insert_key_to_index(off_t offset, const key_t &key, off_t old, off_t after, bool is_leaf);

    void insert_key_to_index_no_split(internal_node_t *node, const key_t &key, off_t value);
    // 统一修改 internal node里一段的parent，在split的时候会出现前后节点连续赋值
    void reset_index_children_parent(index_t *begin, index_t *end, off_t parent);

    // find leaf operation
    // off_t search_leaf_offset(const key_t &key) const;
    // int search_leaf(const key_t &key, leaf_node_t *leaf) const {
    //     return map_block(leaf, search_leaf_offset(key));
    // }
    
    // multi-level文件操作 multable关键词突破const的限制，可处于一种可变状态
    mutable FILE *fp;
    mutable int fp_level; // 这参数啥意思？
    void open_file() const {
        if (fp_level == 0) {
        fp = fopen(path, "rb+");
        if (!fp) { // maybe遇到意外情况，没有正常获得fp指针
            fp = fopen(path, "wb+");
        }
    }
        ++fp_level;
    }
    void close_file() const {
        if (fp_level == 1)
            fclose(fp);
        --fp_level;
    }

    // alloc from disk, program with template
    template<class T>
    off_t dalloc(T *leaf) {
        off_t slot = meta.slot;
        meta.slot += sizeof(leaf_node_t);
        return slot;
    }
    // alloc one leaf from disk
    off_t alloc(leaf_node_t *leaf) {
        leaf->next = 0;
        leaf->n = 0;
        meta.leaf_node_num += 1;
        return dalloc(leaf);
    }

    // alloc one index from disk
    off_t alloc(internal_node_t *node) {
        node->parent = 0;
        node->n = 0;
        meta.internal_node_num += 1;
        return dalloc(node);
    }
    // 手动释放空间
    off_t unalloc(leaf_node_t *leaf);
    off_t unalloc(internal_node_t *node);

    // read block from disk
    template<class T>
    void map(T *block, off_t offset) const {
        open_file();
        fseek(fp, offset, SEEK_SET);
        fread(block, sizeof(T), 1, fp);
        close_file();
    }
    // write block to disk
    template<class T>
    void unmap(T *block, off_t offset) const {
        open_file();
        fseek(fp, offset, SEEK_SET);
        fwrite(block, sizeof(T), 1, fp);
        close_file();
    }

};

}

#endif /* end of BPT_H*/