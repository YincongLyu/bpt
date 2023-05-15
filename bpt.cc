#include "bpt.h"
#include <stdlib.h>
#include <assert.h>

#include <algorithm>

//offset
#define OFFSET_META 0
#define OFFSET_INDEX OFFSET_META + sizeof(meta_t)
#define OFFSET_BLOCK OFFSET_INDEX + meta.index_size
#define OFFSET_END OFFSET_BLOCK + meta.leaf_node_num * sizeof(leaf_node_t)


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
        snyc_meta();
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

        // size_t i;
        // int ret;
        // BINARY_SEARCH(node.children, node.n, key, i, ret);
        //org += node.children[i].child;//但这里也只有一个internal_node
        index_t *r = std::lower_bound(node.children, node.children + node.n, key);
        org += r->child;
        --height; // 这里每层好像只有一个节点
    }
    return offset;
}

void bplus_tree::open_file() const {
    if (fp_level == 0) {
        fp = fopen(path, "rb+");
        if (!fp) { // maybe遇到意外情况，没有正常获得fp指针
            fp = fopen(path, "wb+");
        }
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
    fseek(fp, 0, SEEK_SET);
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
    fseek(fp, OFFSET_META, SEEK_SET);
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

    if (leaf->next == -1) {
        // write new leaf at the end 新leaf物理上追加在end
        write_leaf_to_disk(leaf, OFFSET_END);
    } else {
        // TODO 新的leaf节点插在中间
    }

    // increase the leaf counter
    meta.leaf_node_num++;
    write_meta_to_disk();
    close_file();
}
//当前test只关心 function是否执行成功
int bplus_tree::search(const key_t &key, value_t *value) const {
    leaf_node_t leaf;
    // 定位到指定的leaf block
    map_block(&leaf, search_leaf_offset(key));
    // 根据key找到record
    record_t *record = std::lower_bound(leaf.children, leaf.children + leaf.n, key);
    if (record != leaf.children + leaf.n) {
        *value = record->value;
        return keycmp(record->key, key);
    } else {
        return -1;
    }
}


int bplus_tree::insert(const key_t &key, value_t value) {
    leaf_node_t leaf;
    map_block(&leaf, search_leaf_offset(key));
    
    if (leaf.n == meta.order - 1) {
        // 插入的情况正好满了，需split
        // TODO split when full
    } else {
        // insert into array when leaf is not full
        record_t *r = std::upper_bound(leaf.children, leaf.children + leaf.n, key);
        if (r != leaf.children + leaf.n) {
            // same key?
            if (keycmp(r->key, key) == 0)
                return 1;
            // 后面的统一往后移
            std::copy(r, leaf.children + leaf.n, r + 1);
        }
        r->key = key;
        r->value = value;
        leaf.n++;
    }
    // insert 后 save一下
    write_leaf_to_disk(&leaf, search_leaf_offset(key));
    return value;

}



}