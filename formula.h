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

#define LITERAL_NULL 0U

// Allow only 2^11 literals to make variable set comparison simpler:
#define NUM_LITERALS 2048U

bool LITERAL_eq_value(const literal_t* el1, const literal_t* el2)
{
    return LITERAL_VALUE_Get(*el1) == LITERAL_VALUE_Get(*el2);
}

bool LITERAL_eq_contrarity(const literal_t* el1, const literal_t* el2)
{
    return (~LITERAL_DECISION_BIT & *el1) == (~LITERAL_DECISION_BIT & *el2);
}

bool LITERAL_lt(const literal_t* el1, const literal_t* el2)
{
    return LITERAL_VALUE_Get(*el1) < LITERAL_VALUE_Get(*el2);
}

int LITERAL_value(literal_t lit)
{
    int value = LITERAL_VALUE_Get(lit);

    return (lit & LITERAL_CONTRARY_BIT)? -value : value;
}

//=======================//
// Set of used variables //
//=======================//

#define NUM_SLOTS    64U
#define NUM_SUBSLOTS 32U

// Variable set representation:
// - Variable is represented by a subslot of 2 bits;
// - Each 16 variables are packed into a slot;
// - 64 slots make up a whole set of variables;
typedef struct {
    uint32_t       used[NUM_SLOTS];
    uint32_t contrarity[NUM_SLOTS];
} VARIABLES;

void VARIABLES_init(VARIABLES* vars)
{
    memset(vars->used,       0U, NUM_SLOTS * sizeof(uint32_t));
    memset(vars->contrarity, 0U, NUM_SLOTS * sizeof(uint32_t));
}

bool VARIABLES_equal(const VARIABLES* a, const VARIABLES* b)
{
    return memcmp(a->used, b->used, NUM_SLOTS * sizeof(uint32_t)) == 0U;
}

literal_t VARIABLES_pop_asserted(VARIABLES* vars)
{
    for (uint16_t slot = 0U; slot < NUM_SLOTS; ++slot)
    {
        if (vars->used[slot] != 0U)
        {
            for (unsigned subslot = 0U; subslot < NUM_SUBSLOTS; ++subslot)
            {
                bool used = vars->used[slot] & BIT_MASK(subslot);
                if (used)
                {
                    literal_t lit = 0U;
                    LITERAL_VALUE_Set(lit, slot * NUM_SUBSLOTS + subslot);

                    vars->used[slot]       &= ~BIT_MASK(subslot);
                    vars->contrarity[slot] &= ~BIT_MASK(subslot);

                    return lit;
                }
            }
        }
    }

    return LITERAL_NULL;
}

void VARIABLES_assert_literal(VARIABLES* vars, literal_t lit)
{
    bool contrary = !!(lit & LITERAL_CONTRARY_BIT);

    uint16_t val     = LITERAL_VALUE_Get(lit);
    uint16_t slot    = val / NUM_SUBSLOTS;
    uint16_t subslot = val % NUM_SUBSLOTS;

    // Use subslot and set contrarity:
    vars->used[slot]       |= BIT_MASK(subslot);
    vars->contrarity[slot] |= (contrary << subslot);
}

void VARIABLES_remove_literal(VARIABLES* vars, literal_t lit)
{
    uint16_t val     = LITERAL_VALUE_Get(lit);
    uint16_t slot    = val / NUM_SUBSLOTS;
    uint16_t subslot = val % NUM_SUBSLOTS;

    vars->used[slot]       &= ~BIT_MASK(subslot);
    vars->contrarity[slot] &= ~BIT_MASK(subslot);
}

bool VARIABLES_literal_is_true(const VARIABLES* vars, literal_t lit)
{
    uint16_t val     = LITERAL_VALUE_Get(lit);
    uint16_t slot    = val / NUM_SUBSLOTS;
    uint16_t subslot = val % NUM_SUBSLOTS;

    bool used = vars->used[slot] & BIT_MASK(subslot);

    bool contrarity_lit = !!(lit & LITERAL_CONTRARY_BIT);
    bool contrarity_var = !!(vars->contrarity[slot] & BIT_MASK(subslot));

    return used && contrarity_lit == contrarity_var;
}

bool VARIABLES_literal_is_false(const VARIABLES* vars, literal_t lit)
{
    uint16_t val     = LITERAL_VALUE_Get(lit);
    uint16_t slot    = val / NUM_SUBSLOTS;
    uint16_t subslot = val % NUM_SUBSLOTS;

    bool used = vars->used[slot] & BIT_MASK(subslot);

    bool contrarity_lit = !!(lit & LITERAL_CONTRARY_BIT);
    bool contrarity_var = !!(vars->contrarity[slot] & BIT_MASK(subslot));

    return used && contrarity_lit != contrarity_var;
}

bool VARIABLES_literal_is_undef(const VARIABLES* vars, literal_t lit)
{
    uint16_t val     = LITERAL_VALUE_Get(lit);
    uint16_t slot    = val / NUM_SUBSLOTS;
    uint16_t subslot = val % NUM_SUBSLOTS;

    bool used = vars->used[slot] & BIT_MASK(subslot);
    return !used;
}

void VARIABLES_print(const VARIABLES* vars)
{
    for (uint16_t slot = 0U; slot < NUM_SLOTS; ++slot)
    {
        printf("%04x ", vars->used[slot]);

        // if (vars->used[slot] != 0U)
        // {
        //     for (unsigned subslot = 0U; subslot < NUM_SUBSLOTS; ++subslot)
        //     {
        //         bool used = vars->used[slot] & BIT_MASK(subslot);
        //         if (used)
        //         {
        //             int value = slot * NUM_SUBSLOTS + subslot;
        //             bool contrarity = vars->contrarity[slot] & BIT_MASK(subslot);

        //             printf("%3d ", contrarity? -value : value);
        //         }
        //     }
        // }
    }

    printf("\n");
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
        &LITERAL_eq_value,
        &LITERAL_lt,
        false /*sorted*/);
}

void CLAUSE_free(CLAUSE* clause)
{
    LIT_STORAGE_free(&clause->literals);
}

void CLAUSE_insert(CLAUSE* clause, literal_t element)
{
    LIT_STORAGE_push(&clause->literals, element);
}

void CLAUSE_remove(CLAUSE* clause, size_t index)
{
    literal_t dumpster;
    LIT_STORAGE_remove(&clause->literals, &dumpster, index);
}

size_t CLAUSE_size(const CLAUSE* clause)
{
    return clause->literals.size;
}

literal_t CLAUSE_get(const CLAUSE* clause, size_t index)
{
    return LIT_STORAGE_get(&clause->literals, index);
}

bool CLAUSE_eq(const CLAUSE* el1, const CLAUSE* el2)
{
    return el1 == el2;
}

bool CLAUSE_lt(const CLAUSE* el1, const CLAUSE* el2)
{
    return CLAUSE_size(el1) < CLAUSE_size(el2);
}

void CLAUSE_print(const CLAUSE* clause)
{
    for (size_t lit_i = 0U; lit_i < CLAUSE_size(clause); ++lit_i)
    {
        literal_t lit = CLAUSE_get(clause, lit_i);

        printf("%5d ", LITERAL_value(lit));
    }

    printf("\n");
}

//------------
// DPLL logic
//------------

literal_t CLAUSE_watch1(const CLAUSE* clause)
{
    VERIFY_CONTRACT(
        CLAUSE_size(clause) >= 2,
        "[%s] Clause holds less then two literals", "CLAUSE_watch1");

    return CLAUSE_get(clause, 0U);
}

literal_t CLAUSE_watch2(const CLAUSE* clause)
{
    VERIFY_CONTRACT(
        CLAUSE_size(clause) >= 2,
        "[%s] Claus holds less then two literals", "CLAUSE_watch2");

    return CLAUSE_get(clause, 1U);
}

void CLAUSE_set_watch2(CLAUSE* clause, size_t index)
{
    LIT_STORAGE_swap(&clause->literals, 1U, index);
}

void CLAUSE_swap_watches(CLAUSE* clause)
{
    VERIFY_CONTRACT(
        CLAUSE_size(clause) >= 2,
        "[CLAUSE_watch2] Clause holds less then two literals (size=%zu)",
        CLAUSE_size(clause));

    LIT_STORAGE_swap(&clause->literals, 0U, 1U);
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
        true /*sorted*/);

    VARIABLES_init(&formula->variables);
}

void FORMULA_free(FORMULA* formula)
{
    CLAUSE_STORAGE_free(&formula->clauses);
}

void FORMULA_insert(FORMULA* formula, CLAUSE clause)
{
    CLAUSE_STORAGE_insert_sorted(&formula->clauses, clause);

    for (size_t lit_i = 0U; lit_i < CLAUSE_size(&clause); ++lit_i)
    {
        literal_t lit = CLAUSE_get(&clause, lit_i);

        VARIABLES_assert_literal(&formula->variables, lit & ~LITERAL_CONTRARY_BIT);
    }
}

size_t FORMULA_size(const FORMULA* formula)
{
    return formula->clauses.size;
}

CLAUSE* FORMULA_get(const FORMULA* formula, size_t index)
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
