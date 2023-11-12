// No copyright. Vladislav Aleinik, 2023

#include <stdlib.h>

#include "dimacs.h"

// Convenient naming:
typedef enum
{
    UNSAT = 0,
    SAT   = 1,
    UNDEF = 2
} sat_t;

//================================//
// Assertion trial data structure //
//================================//

typedef struct {
    // Asserted literals:
    LIT_STORAGE literals;

    // Current level:
    uint32_t level;

    // Variables used in current trial:
    VARIABLES variables;
} TRIAL;

void TRIAL_init(TRIAL* trial)
{
    LIT_STORAGE_init(
        &trial->literals,
        &LITERAL_eq_contrarity,
        &LITERAL_lt,
        false);
    trial->level = 0U;

    VARIABLES_init(&trial->variables);
}

void TRIAL_free(TRIAL* trial)
{
    LIT_STORAGE_free(&trial->literals);
}

// Current level for a trial - number of decision literals in it.
// trial_cur_level(trial) = length(trial_decisions(trial))
uint32_t TRIAL_cur_level(const TRIAL* trial)
{
    return trial->level;
}

bool TRIAL_literal_is_true(const TRIAL* trial, literal_t lit)
{
    return VARIABLES_literal_is_true(&trial->variables, lit);
}

bool TRIAL_literal_is_false(const TRIAL* trial, literal_t lit)
{
    return VARIABLES_literal_is_false(&trial->variables, lit);
}

// Checks whether a given assertion trial unsatisfies a formula:
bool TRIAL_formula_is_unsat(const TRIAL* trial, const FORMULA* formula)
{
    // ToDo
}

// Add new literal into a trial:
void TRIAL_assert_literal(TRIAL* trial, literal_t literal)
{
    // Put decision into the literal:
    LIT_STORAGE_push(&trial->literals, literal);

    if (literal & LITERAL_DECISION_BIT)
    {
        trial->level += 1U;
    }

    VARIABLES_assert_literal(trial->variables, literal);
}

void TRIAL_pop_to_last_decision(TRIAL* trial, literal_t* literal)
{
    do
    {
        bool ret = LIT_STORAGE_pop(&trial->literals, literal);
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
    /*  ToDo: search for unit clause.
        \exists clause c;
        \exists literal_t l;
            c \in formula || clause_is_unit(c, l, trial)*/
    )
    {
        TRIAL_assert_literal(trial, l);
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
    while (!TRIAL_formula_is_unsat(trial, formula) && ret != false);
}

//
// Formula preprocessing
//

sat_t dpll_preprocess_formula(const FORMULA* initial, FORMULA* resulting, TRIAL* trial)
{
    // Initialize the resulting formula:
    FORMULA_init(resulting);

    for (size_t cls_i = 0U; cls_i < FORMULA_size(initial); cls_i++)
    {
        // Clause to be preprocessed:
        const CLAUSE* clause = FORMULA_get_ptr(initial, cls_i);

        // Preprocessed clause to be inserted:
        CLAUSE rslt_clause;
        CLAUSE_init(&rslt_clause);

        // Iterate over literals of a clause and copy them to rslt_clause:
        bool insert_clause = true;
        literal_t last_copied = 0U;
        for (size_t lit_i = 0U; lit_i < CLAUSE_size(clause); lit_i++)
        {
            literal_t cur = CLAUSE_get(clause, lit_i);

            // Do not copy falsified literal:
            if (TRIAL_literal_is_false(trial, cur))
            {
                continue;
            }

            // Remove already satisfied clause:
            if (TRIAL_literal_is_true(trial, cur))
            {
                insert_clause = false;
                break;
            }

            // Handle duplicates and tautology:
            if (CLAUSE_size(&rslt_clause) != 0U && LITERAL_eq_value(&last_copied, &cur))
            {
                // Detect clause tautology:
                if (!LITERAL_eq_contrarity(&last_copied, &cur))
                {
                    // Remove tautological clause:
                    insert_clause = false;
                    break;
                }

                // Do not copy duplicate literals:
                continue;
            }

            // Add literal to the clause:
            CLAUSE_insert(&resulting, cur);
        }

        // Handle non-inserted clause:
        if (!insert_clause)
        {
            CLAUSE_free(&rslt_clause);
            continue;
        }

        // Detect UNSAT:
        if (CLAUSE_size(&rslt_clause) == 0U)
        {
            return UNSAT;
        }

        // Assert obvious literal:
        if (CLAUSE_size(&rslt_clause) == 1U)
        {
            TRIAL_assert_literal(trial, last_copied);
            dpll_exhaustive_unit_propagate(trial, resulting);

            continue;
        }

        // NOTE: it is guaranteed that size(rslt_clause) >= 2
        FORMULA_insert(resulting, rslt_clause);
    }

    if (FORMULA_size(resulting) == 0U)
    {
        return SAT;
    }

    return UNDEF;
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

    TRIAL_assert_literal(trial, literal | LITERAL_DECISION_BIT);
}

//
// Backtracking scheme
//
void dpll_apply_backtrack(TRIAL* trial)
{
    // Pop everything to last decision literal:
    literal_t last_decision;

    TRIAL_pop_to_last_decision(trial, &last_decision);

    // Assert literal with reversed contrarity as non-decision:
    last_decision ^=  LITERAL_CONTRARY_BIT;
    last_decision &= ~LITERAL_DECISION_BIT;

    TRIAL_assert_literal(trial, last_decision);
}

//
// General solver algorithm
//
sat_t dpll_solve(const FORMULA* intial_formula)
{
    // Assertion trial:
    TRIAL trial;
    TRIAL_init(&trial);

    // Satisfiability status:
    sat_t sat_flag = UNDEF;

    // Perform initial preprocessing for the formula:
    // NOTE: it is required to initialize invariants
    //       for the Two Watch Literal Scheme
    FORMULA formula;

    sat_flag = dpll_preprocess_formula(intial_formula, &formula, &trial);

    // DPLL algorithm:
    while (sat_flag == UNDEF)
    {
        // Optimize the search by unit propagation:
        dpll_exhaustive_unit_propagate(&trial, &formula);

        if (TRIAL_formula_is_unsat(&trial, &formula))
        {
            if (TRIAL_cur_level(&trial) == 0U)
            {
                // Formula is unsatisfiable with no substitutions => UNSAT.
                sat_flag = UNSAT;
            }
            else
            {
                // Pop substitution from the trial:
                TRIAL_apply_backtrack(&trial);
            }
        }
        else
        {
            const VARIABLES* formula_vars = &formula->variables;
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

    TRIAL_free(&trial);

    return sat_flag;
}

//=======================//
// Assembled DPLL-solver //
//=======================//

int main(int argc, char* argv[])
{
    // Parse input arguments:
    VERIFY_CONTRACT(argc == 2,
        "Usage: %s ./path/to/file.cnf\n", argv[0]);

    FORMULA to_solve;
    DIMACS_load_formula(argv[1], &to_solve);

    sat_t ret = dpll_solve(&to_solve);

    printf("%s", ret == SAT? "SAT" : "UNSAT");

    FORMULA_free(&to_solve);

    return EXIT_SUCCESS;
}
