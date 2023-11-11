// No copyright. Vladislav Aleinik, 2023
#ifndef DPLL_FORMULA_H
#define DPLL_FORMULA_H

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "utils.h"

//===================//
// Literal data type //
//===================//

// Literal representation:
// Bit     15 - literal is used in decision rule
// Bit     14 - literal is set to true
// Bits 11:13 - unsused
// Bits  0:10 - literal number
typedef uint16_t literal_t;

#define LITERAL_DECISION_BIT    BIT_MASK(15U)
#define LITERAL_CONTRARY_BIT    BIT_MASK(14U)

#define LITERAL_VALUE_Get(lit)        READ_BITS(lit,      0U, 10U)
#define LITERAL_VALUE_Set(lit, val) MODIFY_BITS(lit, val, 0U, 10U)

// Allow only 2^11 literals to make variable set comparison simpler:
#define NUM_LITERALS 2048U

bool LITERAL_eq(literal_t* el1, literal_t* el2)
{
    return LITERAL_VALUE_Get(*el1) == LITERAL_VALUE_Get(*el2);
}

bool LITERAL_eq_contrarity(literal_t* el1, literal_t* el2)
{
    return (~LITERAL_DECISION_BIT & *el1) == (~LITERAL_DECISION_BIT & *el2);
}

bool LITERAL_lt(literal_t* el1, literal_t* el2)
{
    return LITERAL_VALUE_Get(*el1) < LITERAL_VALUE_Get(*el2);
}

//=======================//
// Set of used variables //
//=======================//

#define SUBSLOT_BITSHIFT(subslot)     (2U * ((subslot) & 0xFU))
#define SUBSLOT_USED_BIT(subslot)     BIT_MASK(SUBSLOT_BITSHIFT(subslot) + 1U)
#define SUBSLOT_CONTRARY_BIT(subslot) BIT_MASK(SUBSLOT_BITSHIFT(subslot) + 0U)

#define NUM_SLOTS    128U
#define NUM_SUBSLOTS 16U

// Variable set representation:
// - Variable is represented by a subslot of 2 bits;
// - Each 16 variables are packed into a slot;
// - 128 slots make up a whole set of variables;
typedef struct {
    uint32_t slots[NUM_SLOTS];
} VARIABLES;

void VARIABLES_init(VARIABLES* vars)
{
    memset(vars->slots, 0U, NUM_SLOTS * sizeof(uint32_t));
}

bool VARIABLES_equal(const VARIABLES* a, const VARIABLES* b)
{
    return memcmp(a->slots, b->slots, NUM_SLOTS * sizeof(uint32_t)) == 0U;
}

void VARIABLES_assert_literal(VARIABLES* vars, literal_t lit)
{
    bool contrary = !!(lit & LITERAL_CONTRARY_BIT);

    uint16_t val     = LITERAL_VALUE_Get(lit);
    uint16_t slot    = val / NUM_SUBSLOTS;
    uint16_t subslot = val % NUM_SUBSLOTS;

    // Use subslot and set contrarity:
    vars->slots[slot] |= SUBSLOT_USED_BIT(subslot);
    vars->slots[slot] |= (contrary << SUBSLOT_BITSHIFT(subslot));
}

void VARIABLES_remove_literal(VARIABLES* vars, literal_t lit)
{
    uint16_t val     = LITERAL_VALUE_Get(lit);
    uint16_t slot    = val / NUM_SUBSLOTS;
    uint16_t subslot = val % NUM_SUBSLOTS;

    vars->slots[slot] &= ~SUBSLOT_USED_BIT(subslot);
    vars->slots[slot] &= ~SUBSLOT_CONTRARY_BIT(subslot);
}

//=======================//
// Clause data structure //
//=======================//

// Parametrize stack with literal type:
#define DATA_T         literal_t
#define DATA_STRUCTURE LIT_STORAGE
#include "template_stack.h"

typedef struct
{
    LIT_STORAGE literals;
} CLAUSE;

void CLAUSE_init(CLAUSE* clause)
{
    LIT_STORAGE_init(
        &clause->literals,
        &LITERAL_eq,
        &LITERAL_lt,
        true /*sorted*/);
}

void CLAUSE_free(CLAUSE* clause)
{
    LIT_STORAGE_free(&clause->literals);
}

void CLAUSE_insert(CLAUSE* clause, literal_t element)
{
    LIT_STORAGE_insert_sorted(&clause->literals, element);
}

size_t CLAUSE_size(const CLAUSE* clause)
{
    return clause->literals.size;
}

literal_t CLAUSE_get(CLAUSE* clause, size_t index)
{
    return LIT_STORAGE_get(&clause->literals, index);
}

bool CLAUSE_eq(CLAUSE* el1, CLAUSE* el2)
{
    return el1 == el2;
}

bool CLAUSE_lt(CLAUSE* el1, CLAUSE* el2)
{
    return false;
}

void CLAUSE_print(CLAUSE* clause)
{
    for (size_t lit_i = 0U; lit_i < CLAUSE_size(clause); ++lit_i)
    {
        literal_t lit = CLAUSE_get(clause, lit_i);

        int value = (lit & LITERAL_CONTRARY_BIT)?
                    -LITERAL_VALUE_Get(lit) :
                     LITERAL_VALUE_Get(lit);

        printf("%5d ", value);
    }

    printf("\n");
}

//========================//
// Formula data structure //
//========================//

// Parametrize stack with clause data type:
#define DATA_T         CLAUSE
#define DATA_STRUCTURE CLAUSE_STORAGE
#include "template_stack.h"

typedef struct
{
    CLAUSE_STORAGE clauses;

    // Variables used in a formula:
    VARIABLES variables;
} FORMULA;

void FORMULA_init(FORMULA* formula)
{
    CLAUSE_STORAGE_init(&formula->clauses,
        CLAUSE_eq,
        CLAUSE_lt,
        false /*unsorted*/);

    VARIABLES_init(&formula->variables);
}

void FORMULA_free(FORMULA* formula)
{
    CLAUSE_STORAGE_free(&formula->clauses);
}

void FORMULA_insert(FORMULA* formula, CLAUSE clause)
{
    CLAUSE_STORAGE_push(&formula->clauses, clause);

    for (size_t lit_i = 0U; lit_i < CLAUSE_size(&clause); ++lit_i)
    {
        literal_t lit = CLAUSE_get(&clause, lit_i);

        VARIABLES_assert_literal(&formula->variables, lit);
    }
}

size_t FORMULA_size(FORMULA* formula)
{
    return formula->clauses.size;
}

CLAUSE* FORMULA_get(FORMULA* formula, size_t index)
{
    return CLAUSE_STORAGE_get_ptr(&formula->clauses, index);
}

void FORMULA_print(FORMULA* formula)
{
    for (size_t cls_i = 0U; cls_i < FORMULA_size(formula); ++cls_i)
    {
        CLAUSE* cls = FORMULA_get(formula, cls_i);

        printf("[CLAUSE %5ld] ", cls_i);

        CLAUSE_print(cls);
    }
}

#endif // DPLL_FORMULA_H
