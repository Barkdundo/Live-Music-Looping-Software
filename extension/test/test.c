#include "test.h"

#include <stdio.h>

void print_space(char *test_type) {
    puts("\n=====================\n");
    printf("running tests for %s\n", test_type);
    puts("\n=====================\n");
}

int main(void) {
    // print_space("track");
    // test_set_bpm();
    // test_change_key();
    print_space("audio");
    test_audio();
    // print_space("queue");
    // test_queue();
    // print_space("wav");
    // test_wav();
    // print_space("input");
    // test_input();
    return 0;
}
