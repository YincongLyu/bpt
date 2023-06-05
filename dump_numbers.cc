#include "bpt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * command: ./dump_numbers [file_path] [start] [end]
*/
int main(int argc, char *argv[]) {
    char path[512] = {0};
    int start;
    int end;

    if (argc < 4) {
        fprintf(stderr, "argument is not completed, please focus on the given command fomatter\n");
        return 1;
    }

    if (argc > 1) strcpy(path, argv[1]);
    if (argc > 2) start = atoi(argv[2]);
    if (argc > 3) end = atoi(argv[3]);
    // 目前无效命令的格式，参数没有4个，以及start>=end
    if (start >= end) {
        fprintf(stderr, "usage: %s database [%d] [%d]\n", argv[0], start, end);
        return 1;
    }

    // generate numbers and insert into database
    bpt::bplus_tree database("numbers.db", true);
    for (int i = start; i <= end; i++) {
        if (i % 1000 == 0) printf("%d\n", i);
        char key[16] = {0};
        sprintf(key, "%d", i);
        database.insert(key, i);
    }
    printf("%d\n", end);
    printf("done\n");

    return 0;
}