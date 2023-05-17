#ifndef PREDEFINED_H
#define PREDEFINED_H

#include <string.h>

namespace bpt {

// set Bplus-tree index/record 为4个一组
#ifndef BP_ORDER
    #define BP_ORDER 4
#endif

//offset
#define OFFSET_META 0
#define OFFSET_INDEX OFFSET_META + sizeof(meta_t)
#define OFFSET_INDEX_END OFFSET_INDEX + meta.internal_node_num * sizeof(internal_node_t)
#define OFFSET_BLOCK OFFSET_INDEX + meta.index_size
#define OFFSET_END OFFSET_BLOCK + meta.leaf_node_num * sizeof(leaf_node_t)

typedef int value_t;
// 扩展key_t类型为结构体，定义cmp规则
struct key_t{
    char k[32];
    // 若外界有参数传入，则赋值给k
    key_t(const char *str = "") {
        strcpy(k, str);
    }
};

//封装key比较规则，后面使用的key都是key_t类型
inline int keycmp(const key_t &l, const key_t &r) {
    return strcmp(l.k, r.k);
}

// index and record, two type compare rule
#define OPERATOR_KEYCMP(type) \
    bool operator < (const key_t &l, const type &r) {\
        return keycmp(l, r.key) < 0;\
    }\
    bool operator < (const type &l, const key_t &r) {\
        return keycmp(l.key, r) < 0;\
    }
}

#endif /*end of PREDIFINED)H*/