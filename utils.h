// No copyright. Vladislav Aleinik, 2023
#ifndef DPLL_UTILS_H
#define DPLL_UTILS_H

#include <stdlib.h>
#include <stdio.h>

//====================//
// Input verification //
//====================//

#ifndef NDEBUG
#define VERIFY_CONTRACT(contract, format, ...) \
    do { \
        if (!(contract)) { \
            printf((format), ##__VA_ARGS__); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)
#else
#define VERIFY_CONTRACT(contract, format, ...) \
    do {} while (0)
#endif

#define BUG_ON(contract, format, ...) \
    VERIFY_CONTRACT(!(contract), format, ##__VA_ARGS__)


//================//
// Bit operations //
//================//

#define MASK(low, up) \
    ((((up) == 31U)? -1U : ((1U << ((up) + 1U)) - 1U)) ^ ((1U << (low)) - 1U))

#define BIT_MASK(bit) \
    (1U << (bit))

#define MODIFY_BITS(reg, val, low, up) \
    ((reg) = ((reg) & ~MASK(low, up)) | ((((uint16_t) (val)) << (low)) & MASK(low, up)))

#define READ_BITS(reg, low, up) \
    (((reg) >> (low)) & ((1U << ((up) - (low) + 1U)) - 1U))

//===============//
// Miscellaneous //
//===============//

#define MIN(a, b) ((a) < (b)? (a) : (b))

#define STR(token) #token

#endif // DPLL_UTILS_H
