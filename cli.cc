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
        value_t value;
        if (database.search(argv[3], &value) != 0) {
            printf("Key %s not found\n", argv[3]);
        } else {
            printf("%d\n", value);
        }
    }
    return 0;
}