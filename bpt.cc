#include "bpt.h"
#include <stdlib.h>
#include <assert.h>

#define BINARY_SEARCH(array, n, key, i, ret) {\
    size_t l = 0, r = n;\
    size_t mid;\
    while (l < n) {\
        mid = l + ((r - l) >> 1);\
        ret = keycmp(key, array[mid].key);\
        if (ret < 0) r = mid;\
        else if (ret > 0) l = mid;\
        else break;\
    }\
    i = mid;\
}

namespace bpt {

/**
 * 构造函数, 顺便初始化fp和fp_level
 * 入参：文件路径 + 可指定是否强制为空树
*/
bplus_tree::bplus_tree(const char *p, bool force_empty) : fp(NULL), fp_level(0) {
    // 清零，并对成员path赋值外界传入的文件路径
    bzero(path, sizeof(path));
    strcpy(path, p);

    FILE *fp = fopen(path, "r");
    fclose(fp); // 打开后关闭，但这里是否释放了fp对应的内存空间？
    if (!fp || force_empty) {
        //如果文件不存在的话，就新建一颗空树
        init_from_empty();
    } else {
        // 如果路径p文件已经是一颗tree，那就同步下meta信息
        snyc_meta();
    }
}

/**
 * 构建空树: 初始化meta数据+root节点
*/
void bplus_tree::init_from_empty() {
    meta.order = BP_ORDER;
    // 预分配128个内部节点
    meta.index_size = sizeof(internal_node_t) * 128;
    meta.value_size = sizeof(value_t);
    meta.key_size = sizeof(key_t);
    meta.internal_node_num = meta.leaf_node_num = meta.height = 0;
    write_meta_to_disk();

    //初始化root节点
    leaf_node_t leaf;
    leaf.pre = leaf.next = -1;
    leaf.n = 0;
    write_new_leaf_to_disk(&leaf);
}

/**
 * 这函数很重要！
*/
off_t bplus_tree::search_leaf_offset(const key_t &key) const {
    // 这里模拟存储是连续的数组，leaf位置跟在64个内部节点后面
    off_t offset = sizeof(meta_t) + meta.index_size;
    int height = meta.height;
    off_t org = sizeof(meta_t);
    // 先走到leaf节点
    while (height > 0) {
        // 获得当前node, 走过meta的offset
        internal_node_t node;
        map_index(&node, org);

        size_t i;
        int ret;
        BINARY_SEARCH(node.children, node.n, key, i, ret);
        org += node.children[i].child;//但这里也只有一个internal_node

        --height;
    }
    return offset;
}

void bplus_tree::open_file() const {
    if (fp_level == 0) {
        fp = fopen(path, "rb+");
    }
    ++fp_level;
}

void bplus_tree::close_file() const {
    if (fp_level == 1) {
        fclose(fp);
    }
    --fp_level;
}
/**
 * 手动同步元信息用，防止更新不及时
*/
void bplus_tree::snyc_meta() {
    open_file();
    // 从fp里读meta信息，就放在开头
    fread(&meta, sizeof(meta), 1, fp);
    close_file();
}
/**
 * 根据offset定位node位置，直接从文件流里读取node大小的字节
*/
int bplus_tree::map_index(internal_node_t *node, off_t offset) const {
    open_file();
    fseek(fp, offset, SEEK_SET);
    fread(node, sizeof(internal_node_t), 1, fp);
    close_file();
    return 0;
}

int bplus_tree::map_block(leaf_node_t *node, off_t offset) const {
    open_file();
    fseek(fp, offset, SEEK_SET);
    fread(node, sizeof(leaf_node_t), 1, fp);
    close_file();
    return 0;
}

void bplus_tree::write_meta_to_disk() const {
    open_file();
    fwrite(&meta, sizeof(meta_t), 1, fp);
    close_file();
}

void bplus_tree:: write_leaf_to_disk(leaf_node_t *leaf, off_t offset) {
    open_file();
    fseek(fp, offset, SEEK_SET);
    fwrite(&meta, sizeof(leaf_node_t), 1, fp);
    close_file();
}
/**
 * 写新leaf节点与有offset不同的是，需要手动增加其他关于leaf的信息
*/
void bplus_tree:: write_new_leaf_to_disk(leaf_node_t *leaf) {
    open_file();

    meta.leaf_node_num++;
    write_meta_to_disk();
    if (leaf->next == -1) {
        fseek(fp, 0, SEEK_SET); //为什么在末尾加新leaf，的文件偏移量是0?
    } else {
        // TODO 新的leaf节点插在中间
    }

    fwrite(leaf, sizeof(leaf_node_t), 1, fp);
    close_file();
}

value_t bplus_tree::search(const key_t &key) const {
    leaf_node_t leaf;
    // 定位到指定的leaf block
    map_block(&leaf, search_leaf_offset(key));

    size_t i;
    int ret;
    // 在当前的leaf block里顺序遍历，O(n)比较key值
    // for (i = 0; i < leaf.n; ++i) {
    //     //又已知已经有序，到第一个>=key的可以提前停止
    //     // FIX
    //     if ((ret = keycmp(leaf.children[i].key, key)) > 0)
    //         break;
    // }
    BINARY_SEARCH(leaf.children, leaf.n, key, i, ret);
    // 这里>情况终止的value_t()是什么？
    return ret == 0 ? leaf.children[i].value : value_t();
}


value_t bplus_tree::insert(const key_t &key, value_t value) {
    leaf_node_t leaf;
    map_block(&leaf, search_leaf_offset(key));
    
    if (leaf.n == meta.order - 1) {
        // 插入的情况正好满了，需split
        // TODO split when full
    } else {
        size_t i;
        int ret = -1;
        // 插入到哪里?先顺序遍历
        // for (i = 0; i < leaf.n; ++i) {
        //     if ((ret = keycmp(leaf.children[i].key, key)) > 0)
        //         break;
        // }
        BINARY_SEARCH(leaf.children, leaf.n, key, i, ret);

        // if 已经有相同的key了，直接返回value，不允许插入
        if (ret == 0) return leaf.children[i].value;
        // 如果插入的pos在中间，则需移动[i+1, leaf.n)之间的record,往后搬
        if (i < leaf.n) {
            for (size_t j = leaf.n; j > i; --j) {
                // 这里肯定不会越界，line47已经判定了
                leaf.children[j] = leaf.children[j - 1];
            }
        }
        leaf.children[i].key = key;
        leaf.children[i].value = value;
        leaf.n++;
    }
    // insert 后 save一下
    write_leaf_to_disk(&leaf, search_leaf_offset(key));
    return value;

}



}