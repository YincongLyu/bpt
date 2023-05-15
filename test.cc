#include <assert.h>
#include <stdio.h>

#include "bpt.h"
using bpt::bplus_tree;

#define PRINT(a) printf("\033[32m%s\033[0m \033[32m%s\033[0m\n", a, "passed");

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
        assert(empty_tree.meta.key_size == sizeof(bpt::key_t));
        assert(empty_tree.meta.internal_node_num == 0);
        assert(empty_tree.meta.leaf_node_num == 1);
        assert(empty_tree.meta.height == 0);
        PRINT("empty tree");
    }

    {
        // 对于已存在的树，只需同步下meta信息
        bplus_tree existing_tree = bplus_tree("test.db");
        assert(existing_tree.meta.order == 4);
        assert(existing_tree.meta.value_size == sizeof(bpt::value_t));
        assert(existing_tree.meta.key_size == sizeof(bpt::key_t));
        assert(existing_tree.meta.internal_node_num == 0);
        assert(existing_tree.meta.leaf_node_num == 1);
        assert(existing_tree.meta.height == 0);
        PRINT("reread empty tree");

        existing_tree.insert("t2", 2);
        existing_tree.insert("t1", 1);
        existing_tree.insert("t3", 3);

        bpt::leaf_node_t leaf;
        // insert 完后立即查询，是通过insert里的立马write to disk保证的
        existing_tree.search_leaf("t1", &leaf);
        assert(leaf.n == 3);
        // key是否正确
        assert(bpt::keycmp(leaf.children[0].key, "t1") == 0);
        assert(bpt::keycmp(leaf.children[1].key, "t2") == 0);
        assert(bpt::keycmp(leaf.children[2].key, "t3") == 0);
        // value是否正确
        bpt::value_t value;
        assert(existing_tree.search("t1", &value) == 0);
        assert(value == 1);
        assert(existing_tree.search("t2", &value) == 0);
        assert(value == 2);
        assert(existing_tree.search("t3", &value) == 0);
        assert(value == 3);
        assert(existing_tree.search("t4", &value) != 0); // 找不到example
        PRINT("insert 3 elements passed");

    }

 {
        // 对于已存在的树，只需同步下meta信息
        bplus_tree tree = bplus_tree("test.db");
        assert(tree.meta.order == 4);
        assert(tree.meta.value_size == sizeof(bpt::value_t));
        assert(tree.meta.key_size == sizeof(bpt::key_t));
        assert(tree.meta.internal_node_num == 0);
        assert(tree.meta.leaf_node_num == 1);
        assert(tree.meta.height == 0);

        // repeat test search, if there is any bug?
        bpt::leaf_node_t leaf;
        tree.search_leaf("t1", &leaf);
        assert(leaf.n == 3);
        // key是否正确
        assert(bpt::keycmp(leaf.children[0].key, "t1") == 0);
        assert(bpt::keycmp(leaf.children[1].key, "t2") == 0);
        assert(bpt::keycmp(leaf.children[2].key, "t3") == 0);
        // value是否正确
        bpt::value_t value;
        assert(tree.search("t1", &value) == 0);
        assert(value == 1);
        assert(tree.search("t2", &value) == 0);
        assert(value == 2);
        assert(tree.search("t3", &value) == 0);
        assert(value == 3);
        assert(tree.search("t4", &value) != 0); // 找不到example
        PRINT("repeat search passed");

    }

}