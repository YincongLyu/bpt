#include <assert.h>
#include <stdio.h>

#include "bpt.h"
using bpt::bplus_tree;

int main(int argc, char *argv[]) {
   //                    _            _                    _            _   
//   ___ _ __ ___  _ __ | |_ _   _   | |_ _ __ ___  ___   | |_ ___  ___| |_ 
//  / _ \ '_ ` _ \| '_ \| __| | | |  | __| '__/ _ \/ _ \  | __/ _ \/ __| __|
// |  __/ | | | | | |_) | |_| |_| |  | |_| | |  __/  __/  | ||  __/\__ \ |_ 
//  \___|_| |_| |_| .__/ \__|\__, |___\__|_|  \___|\___|___\__\___||___/\__|
//                |_|        |___/_____|              |_____|               

    {
        bplus_tree empty_tree = bplus_tree("test.db", true);
        assert(empty_tree.meta.order == 4);
        assert(empty_tree.meta.value_size == sizeof(bpt::value_t));
        assert(empty_tree.meta.key_size == sizeof(key_t));
        assert(empty_tree.meta.internal_node_num == 0);
        assert(empty_tree.meta.leaf_node_num == 1);
        assert(empty_tree.meta.height == 0);
        printf("\033[32m%s\033[0m\n", "empty tree passed");
    }

    // {
    //     // 对于已存在的树，只需同步下meta信息
    //     bplus_tree existing_tree = bplus_tree("test.db");
    //     assert(existing_tree.meta.order == 4);
    //     assert(existing_tree.meta.value_size == sizeof(bpt::value_t));
    //     assert(existing_tree.meta.key_size == sizeof(key_t));
    //     assert(existing_tree.meta.internal_node_num == 0);
    //     assert(existing_tree.meta.leaf_node_num == 1);
    //     assert(existing_tree.meta.height == 0);
    //     printf("\033[32m%s\033[0m\n", "existing empty tree passed");

    //     existing_tree.insert("t1", 1);
    //     existing_tree.insert("t2", 2);
    //     existing_tree.insert("t3", 3);

    //     bpt::leaf_node_t leaf;
    //     // insert 完后立即查询，是通过insert里的立马write to disk保证的
    //     existing_tree.search_leaf("t1", &leaf);
    //     assert(leaf.n == 3);
    //     // key是否正确
    //     assert(bpt::keycmp(leaf.children[0].key, "t1") == 0);
    //     assert(bpt::keycmp(leaf.children[1].key, "t2") == 0);
    //     assert(bpt::keycmp(leaf.children[2].key, "t3") == 0);
    //     // value是否正确
    //     assert(leaf.children[0].value == 1);
    //     assert(leaf.children[1].value == 2);
    //     assert(leaf.children[2].value == 3);
    //     printf("\033[32m%s\033[0m\n", "insert 3 elements passed");

    // }

//  {
//         // 对于已存在的树，只需同步下meta信息
//         bplus_tree existing_tree = bplus_tree("test.db");
//         assert(existing_tree.meta.order == 4);
//         assert(existing_tree.meta.value_size == sizeof(bpt::value_t));
//         assert(existing_tree.meta.key_size == sizeof(key_t));
//         assert(existing_tree.meta.internal_node_num == 0);
//         assert(existing_tree.meta.leaf_node_num == 1);
//         assert(existing_tree.meta.height == 0);
//         printf("\033[32m%s\033[0m\n", "existing empty tree passed");

//         // repeat test search, if there is any bug?
//         bpt::leaf_node_t leaf;
//         existing_tree.search_leaf("t1", &leaf);
//         assert(leaf.n == 3);
//         // key是否正确
//         assert(bpt::keycmp(leaf.children[0].key, "t1") == 0);
//         assert(bpt::keycmp(leaf.children[1].key, "t2") == 0);
//         assert(bpt::keycmp(leaf.children[2].key, "t3") == 0);
//         // value是否正确
//         assert(leaf.children[0].value == 1);
//         assert(leaf.children[1].value == 2);
//         assert(leaf.children[2].value == 3);
//         printf("\033[32m%s\033[0m\n", "repeat search passed");

//     }

}