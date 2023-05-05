#include <stdio.h>

int main() {
    puts("Trying to crash...");
    *(int*)0xdeadbeef = 0xcafebabe;
    puts("Failed to crash :)");
}

