// No copyright. Vladislav Aleinik, 2023

//===========//
// Utilities //
//===========//

#include "utils.h"

// Convenient naming:
typedef enum
{
    UNSAT = 0,
    SAT   = 1,
    UNDEF = 2
} sat_t;

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

void variables_init(VARIABLES* vars)
{
    memset(vars->slots, 0U, NUM_SLOTS * sizeof(uint32_t));
}

bool variables_equal(const VARIABLES* a, const VARIABLES* b)
{
    return memcmp(a->slots, b->slots, NUM_SLOTS * sizeof(uint32_t)) == 0U;
}

void variables_assert_literal(VARIABLES* vars, literal_t lit)
{
    bool contrary = !!(lit & LITERAL_CONTRARY_BIT);

    uint16_t val     = LITERAL_VALUE_Get(lit);
    uint16_t slot    = val / NUM_SUBSLOTS;
    uint16_t subslot = val % NUM_SUBSLOTS;

    // Use subslot and set contrarity:
    vars->slots[slot] |= SUBSLOT_USED_BIT(subslot);
    vars->slots[slot] |= (contrary << SUBSLOT_BITSHIFT(subslot));
}

void variables_remove_literal(VARIABLES* vars, literal_t lit)
{
    uint16_t val     = LITERAL_VALUE_Get(lit);
    uint16_t slot    = val / NUM_SUBSLOTS;
    uint16_t subslot = val % NUM_SUBSLOTS;

    vars->slots[slot] &= ~SUBSLOT_USED_BIT(subslot);
    vars->slots[slot] &= ~SUBSLOT_CONTRARY_BIT(subslot);
}

//========================//
// Formula data structure //
//========================//

typedef struct
{
    // ToDo

    // Variables used in a formula:
    VARIABLES variables;
} FORMULA;

//===============//
// DIMACS parser //
//===============//

// ToDo

//================================//
// Assertion trial data structure //
//================================//

// Parametrize stack with literal type:
typedef literal_t data_t;
#include "stack.h"

typedef struct {
    // Asserted literals:
    STACK literals;

    // Current level:
    uint32_t level;

    // Variables used in current trial:
    VARIABLES variables;
} TRIAL;

void trial_init(TRIAL* trial)
{
    stack_init(&trial->literals);
    trial->level = 0U;

    variables_init(&trial->variables);
}

void trial_free(TRIAL* trial)
{
    stack_free(&trial->literals);
}

// Current level for a trial - number of decision literals in it.
// trial_cur_level(trial) = length(trial_decisions(trial))
uint32_t trial_cur_level(const TRIAL* trial)
{
    return trial->level;
}

// Checks whether a given assertion trial unsatisfies a formula:
bool trial_is_unsat(const TRIAL* trial, const FORMULA* formula)
{
    // ToDo
}

// Add new literal into a trial:
void trial_assert_literal(TRIAL* trial, literal_t literal)
{
    // Put decision into the literal:
    stack_push(&trial->literals, literal);

    if (literal & LITERAL_DECISION_BIT)
    {
        trial->level += 1U;
    }

    variables_assert_literal(trial->variables, literal);
}

void trial_pop_to_last_decision(TRIAL* trial, literal_t* literal)
{
    do
    {
        bool ret = stack_pop(&trial->stack, literal);
        BUG_ON(!ret, "[trial_pop_to_last_decision] Expected at least one decision literal!")
    }
    while (!(*literal & LITERAL_DECISION_BIT));

    trial->level -= 1U;
}

//================//
// DPLL algorithm //
//================//

//
// Unit propagation
//
bool dpll_apply_unit_propagate(TRIAL* trial, const FORMULA* formula)
{
    if (
    /*  Search for unit clause:
        \exists clause c;
        \exists literal_t l;
            c \in formula || clause_is_unit(c, l, trial)*/
    )
    {
        trial_assert_literal(trial, l);
        return true;
    }

    return false;
}

void dpll_exhaustive_unit_propagate(TRIAL* trial, const FORMULA* formula)
{
    bool ret;
    do
    {
        ret = dpll_apply_unit_propagate(trial, formula);
    }
    while (trial_is_unsat(trial, formula) || ret == false);
}

//
// Branching scheme
//

literal_t dpll_select_literal(const TRIAL* trial, const FORMULA* formula)
{
    // ToDo
}

void dpll_apply_decide(TRIAL* trial, const FORMULA* formula)
{
    literal_t branching_literal =
        dpll_select_literal(trial, formula);

    trial_assert_literal(trial, literal | LITERAL_DECISION_BIT);
}

//
// Backtracking scheme
//
void dpll_apply_backtrack(TRIAL* trial)
{
    // Pop everything to last decision literal:
    literal_t last_decision;

    trial_pop_to_last_decision(trial, &last_decision);

    // Assert literal with reversed contrarity as non-decision:
    last_decision ^=  LITERAL_CONTRARY_BIT;
    last_decision &= ~LITERAL_DECISION_BIT;

    trial_assert_literal(trial, last_decision);
}

//
// General solver algorithm
//
sat_t dpll_solve(const FORMULA* formula)
{
    // Satisfiability status:
    sat_t sat_flag = UNDEF;

    // Assertion trial:
    TRIAL trial;
    trial_init(&trial);

    while (sat_flag == UNDEF)
    {
        // Optimize the search by unit propagation:
        dpll_exhaustive_unit_propagate(&trial, formula);

        if (trial_is_unsat(&trial, formula))
        {
            if (trial_cur_level(&trial) == 0U)
            {
                // Formula is unsatisfiable with no substitutions => UNSAT.
                sat_flag = UNSAT;
            }
            else
            {
                // Pop substitution from the trial:
                trial_apply_backtrack(&trial);
            }
        }
        else
        {
            const VARIABLES* formula_vars = &formula.variables;
            const VARIABLES*   trial_vars = &trial->variables;

            if (variables_equal(formula_vars, trial_vars))
            {
                // Explicitly get the valuation that satisfies the formula => SAT.
                sat_flag = SAT;
            }
            else
            {
                // Use decision to obtain substitution:
                dpll_apply_decide(&trial);
            }
        }
    }

    trial_free(&trial);
}

//=======================//
// Assembled DPLL-solver //
//=======================//

int main(int argc, char* argv[])
{
    // ToDo
}
