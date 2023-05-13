#ifndef BPT_H
#define BPT_H

namespace bpt {

// using key_t[32] = char; 很奇怪这里我没有
typedef char key_t[32];
typedef int value_t;

typedef struct bpt {
    int order;
    int block;
    int key_size;
    int value_size;
} meta_t;
// 为什么这里只把meta_t给外界用了？
extern meta_t meta;

}

#endif /* end of BPT_H*/