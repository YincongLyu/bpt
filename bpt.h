#ifndef BPT_H
#define BPT_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

namespace bpt {

// 扩展key_t类型为结构体，定义cmp规则
//typedef char key_t[32];
struct key_t{
    char k[32];
    key_t() {}
    // 若外界有参数传入，则赋值给k
    key_t(const char *str) {
        strcpy(k, str);
    }
};
//封装key比较规则，后面使用的key都是key_t类型
inline int keycmp(const key_t &l, const key_t &r) {
    return strcmp(l.k, r.k);
}

typedef int value_t;

// 预定义 B+ tree信息
#define BP_ORDER 4

//B+ tree的元数据
struct meta_t {
    size_t order; //B+ tree的顺序
    size_t index_size; // 记录所有的index所占的空间?
    size_t value_size; // value的大小
    size_t key_size; // key的大小
    size_t internal_node_num; // 内部所有节点的数量
    size_t leaf_node_num; // 叶子节点的刷零
    size_t height; // 包括叶子节点那层的树高
};

// 内部节点的index segment
struct index_t {
    key_t key;
    size_t child; // offset，child指什么?
};

// 内部节点的block组织形式，一个block指向下面的孩子节点
/**
 * internal node block
 * | size_t n | key_t key, size_t child | key_t key, size_t child | ... |
*/
struct internal_node_t {
    size_t n; // 有多少孩子节点
    index_t children[BP_ORDER];
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
    int pre;
    int next;
    size_t n; // 当前leaf block所有的record数量
    record_t children[BP_ORDER - 1];
};

class bplus_tree {
public:
    // 构造函数
    bplus_tree(const char *path, bool force_empty = false);

    //抽象函数CRUD
    value_t search(const key_t &key) const;
    value_t erase(const key_t &key);
    value_t insert(const key_t &key, value_t value);
    value_t update(const key_t &key, value_t value);

public:
    char path[512];//文件读写路径

    meta_t meta;

    // 构建一个空树
    void init_from_empty();

    // stdio里的把long typedef了
    // 根据key定位leaf，需调用read模块的map
    off_t search_leaf_offset(const key_t &key) const;
    int search_leaf(const key_t &key, leaf_node_t *leaf) const {
        return map_block(leaf, search_leaf_offset(key));
    }
    
    // multi-level文件操作 multable关键词突破const的限制，可处于一种可变状态
    mutable FILE *fp;
    mutable int fp_level; // 这参数啥意思？
    inline void open_file(const char *mode = "rb+") const;
    inline void close_file() const;

    // 读写data 和disk的交互都是站在tree的逻辑视角，以node为单位
    void snyc_meta();
    int map_index(internal_node_t *node, off_t offset) const;
    int map_block(leaf_node_t *leaf, off_t offset) const;

    // write 写操作我们的视角关注于record，都放于leaf
    void write_meta_to_disk() const;
    void write_leaf_to_disk(leaf_node_t *leaf, off_t offset);
    void write_new_leaf_to_disk(leaf_node_t *leaf);
    void snyc_leaf_to_disk(leaf_node_t *leaf) const;//手动同步leaf的信息
};

}

#endif /* end of BPT_H*/