#include <assert.h>
#include <stdio.h>

#include "../bpt.h"
using bpt::bplus_tree;

#define PRINT(a) printf("\033[32m%s\033[0m \033[32m%s\033[0m\n", a, "passed");

#define BP_ORDER 4

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
        assert(empty_tree.meta.internal_node_num == 1);
        assert(empty_tree.meta.leaf_node_num == 1);
        assert(empty_tree.meta.height == 1);
        assert(empty_tree.meta.block_slot == 1);
        assert(empty_tree.meta.index_slot == 1);
        PRINT("empty tree");
    }

    {
        // 对于已存在的树，只需同步下meta信息
        bplus_tree existing_tree = bplus_tree("test.db");
        assert(existing_tree.meta.order == 4);
        assert(existing_tree.meta.value_size == sizeof(bpt::value_t));
        assert(existing_tree.meta.key_size == sizeof(bpt::key_t));
        assert(existing_tree.meta.internal_node_num == 1);
        assert(existing_tree.meta.leaf_node_num == 1);
        assert(existing_tree.meta.height == 1);
        assert(existing_tree.meta.block_slot == 1);
        assert(existing_tree.meta.index_slot == 1);
        PRINT("reread empty tree");

        assert(existing_tree.insert("t2", 2) == 0);
        assert(existing_tree.insert("t4", 4) == 0);
        assert(existing_tree.insert("t1", 1) == 0);
        assert(existing_tree.insert("t3", 3) == 0);

        // bpt::leaf_node_t leaf;
        // // insert 完后立即查询，是通过insert里的立马write to disk保证的
        // existing_tree.search_leaf("t1", &leaf);
        // assert(leaf.n == 3);
        // // key是否正确
        // assert(bpt::keycmp(leaf.children[0].key, "t1") == 0);
        // assert(bpt::keycmp(leaf.children[1].key, "t2") == 0);
        // assert(bpt::keycmp(leaf.children[2].key, "t3") == 0);
        // // value是否正确
        // bpt::value_t value;
        // assert(existing_tree.search("t1", &value) == 0);
        // assert(value == 1);
        // assert(existing_tree.search("t2", &value) == 0);
        // assert(value == 2);
        // assert(existing_tree.search("t3", &value) == 0);
        // assert(value == 3);
        // assert(existing_tree.search("t4", &value) != 0); // 找不到example
        // PRINT("insert 3 elements passed");

    }

 {
        // 对于已存在的树，只需同步下meta信息
        bplus_tree tree = bplus_tree("test.db");
        assert(tree.meta.order == 4);
        assert(tree.meta.value_size == sizeof(bpt::value_t));
        assert(tree.meta.key_size == sizeof(bpt::key_t));
        assert(tree.meta.internal_node_num == 1);
        assert(tree.meta.leaf_node_num == 1);
        assert(tree.meta.height == 1);

        // repeat test search, if there is any bug?
        bpt::leaf_node_t leaf;
        tree.map(&leaf, tree.search_leaf("t1"));
        assert(leaf.n == 4);
        // key是否正确
        assert(bpt::keycmp(leaf.children[0].key, "t1") == 0);
        assert(bpt::keycmp(leaf.children[1].key, "t2") == 0);
        assert(bpt::keycmp(leaf.children[2].key, "t3") == 0);
        assert(bpt::keycmp(leaf.children[3].key, "t4") == 0);
        // value是否正确
        bpt::value_t value;
        assert(tree.search("t1", &value) == 0);
        assert(value == 1);
        assert(tree.search("t2", &value) == 0);
        assert(value == 2);
        assert(tree.search("t3", &value) == 0);
        assert(value == 3);
        assert(tree.search("t4", &value) == 0);
        assert(value == 4);
        
        assert(tree.insert("t1", 4) == 1);
        assert(tree.insert("t2", 4) == 1);
        assert(tree.insert("t3", 4) == 1);
        assert(tree.insert("t4", 4) == 1);
        PRINT("Insert4Elements");
        assert(tree.insert("t5", 5) == 0);
    }

}