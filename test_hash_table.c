#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash_table.h"

#define TEST_SIZE 30
#define KEYVAL_LEN 128
char test_keys[TEST_SIZE][KEYVAL_LEN] = {
    "1",
    "0100",
    "1001",
    "1101",
    "1010111",
    "0101011",
    "10100111",
    "10101011",
    "101001011",
    "01010010011",
    "10100101001",
    "1110100011111",
    "10010100101011",
    "10001100101011",
    "11111111111100",
    "10010111110101001",
    "10010111111111001",
    "101011111111111100",
    "1111111111110011001",
    "11011001010010100101",
    "0000000000000000010000",
    "1111111111111110011001",
    "1000100011011110101001",
    "1111100101010111110011001",
    "11111110111110001110011001",
    "11111001001111111110011001",
    "1001011111110000110101011101001",
    "00000000000000000100100101010000101011100",
    "1001011111010100101010010100100101010010011",
    "0000000111111111100001010101010000000000010000"
};
unsigned long long int test_vals[TEST_SIZE] = {
    223423452,
    982374019238,
    43289798723,
    348927798234,
    49875,
    6182979,
    123345,
    63,
    1341,
    4534653,
    2452342342,
    123123,
    0,
    1231242345,
    123,
    5463,
    65345231234,
    12,
    64536,
    67542,
    6236,
    654732,
    276345723456,
    6742623562352,
    75474574546363,
    763463453743523465,
    6346737346324,
    34763463735,
    324763476373,
    374573473463,
};

int main() {
    const int size = 123456;
    HashTable *tbl = hash_table_create(size);
    for(int i = 0; i < TEST_SIZE; ++i) {
        hash_table_insert(tbl, test_keys[i], test_vals[i]);
    }
    for(int i = 0; i < TEST_SIZE; ++i) {
        unsigned long long int got_val = hash_table_get(tbl, test_keys[i]);
        if(got_val == test_vals[i]) {
            printf("\x1b[32mSUCCESS\x1b[39m: %s -> %llu\n", test_keys[i],
                   test_vals[i]);
        } else {
            printf("\x1b[31mFAILED\x1b[39m:  %s -> %llu, (correct: %llu)\n", test_keys[i],
                   got_val, test_vals[i]);
            exit(EXIT_FAILURE);
        }
    }
    hash_table_destroy(tbl);
    printf("All tests passed\n");
    return EXIT_SUCCESS;
}