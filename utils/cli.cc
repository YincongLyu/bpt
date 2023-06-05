#include "bpt.h"
#include <string.h>

using namespace bpt;

/**
 * ./bpt_cli [database] insert a 1
 * ./bpt_cli [database] update a 2
 * ./bpt_cli [database] search 2
 * ./bpt_cli [database] search 1 100
*/
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s database command\n", argv[0]);
        return 1;
    }
    bpt::bplus_tree database(argv[1]);
    if (!strcmp(argv[2], "search")) {
        if (argc < 4) {
            fprintf(stderr, "please enter a key\n");
            return 1;
        }
        if (argc == 4) { 
            value_t value;
            if (database.search(argv[3], &value) != 0) {
                printf("key %s not found\n", argv[3]);
            } else {
                printf("%d\n", value);
            }
        } else {
            value_t values[512];
            int ret = database.search_range(argv[3], argv[4], values, 512);
            for (int i = 0; i < ret; i++) {
                printf("%d\n", values[i]);
            }
        }
    } else if (!strcmp(argv[2], "insert")) {
        if (argc < 5) {
            fprintf(stderr, "insert fomatter is [insert key value]");
            return 1;
        }
        if (database.insert(argv[3], atoi(argv[4])) != 0) {
            printf("key %s is already exists\n", argv[3]);
        }
    } else if (!strcmp(argv[2], "update")) {
        if (argc < 5) {
            fprintf(stderr, "update fomatter is [update key value]");
            return 1;
        }
        if (database.update(argv[3], atoi(argv[4])) != 0) {
            printf("key %s doesn't exist\n", argv[3]);
        }
    } else {
        fprintf(stderr, "invalid command: %s\n", argv[2]);
    }
    return 0;
}