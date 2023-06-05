#ifndef PREDEFINED_H
#define PREDEFINED_H

#include <string.h>

namespace bpt {

#ifndef UNIT_TEST
    #define BP_ORDER 20
#else
// set Bplus-tree index/record 为4个一组
    #define BP_ORDER 4
#endif


typedef int value_t;
// 扩展key_t类型为结构体，定义cmp规则
struct key_t{
    char k[16];
    // 若外界有参数传入，则赋值给k
    key_t(const char *str = "") {
        bzero(k, sizeof(k));
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