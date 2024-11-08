#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define BASE58_ALPHABET "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"

// Base58 encoding function for a 32-byte public key
int encode_base58(const uint8_t *input, size_t input_len, char *output, size_t output_size) {
    const char *alphabet = BASE58_ALPHABET;
    size_t zeros = 0;
    size_t i, j;
    size_t size;
    size_t length = 0;
    uint8_t *b58;

    // Count leading zeros
    while (zeros < input_len && input[zeros] == 0) {
        zeros++;
    }

    // Allocate enough space in big-endian base58 representation
    size = (input_len - zeros) * 138 / 100 + 1; // log(256) / log(58), rounded up
    b58 = (uint8_t *)calloc(size, sizeof(uint8_t));
    if (b58 == NULL) {
        return -1; // Memory allocation failed
    }

    // Process the bytes
    for (i = zeros; i < input_len; i++) {
        uint32_t carry = input[i];
        size_t k = size;
        while (carry != 0 || k > size - length) {
            carry += 256 * b58[--k];
            b58[k] = carry % 58;
            carry /= 58;
        }
        length = size - k;
    }

    // Skip leading zeros in b58
    i = size - length;

    // Calculate the required output size
    size_t total_output_length = zeros + length;
    if (total_output_length + 1 > output_size) { // +1 for null terminator
        free(b58);
        return -1; // Output buffer too small
    }

    // Leading '1's for zeros
    memset(output, '1', zeros);

    // Translate the result into a string
    for (j = 0; j < length; j++) {
        output[zeros + j] = alphabet[b58[i + j]];
    }
    output[zeros + length] = '\0';

    free(b58);
    return 0;
}