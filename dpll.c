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

// //================================//
// // Assertion trial data structure //
// //================================//

// typedef struct {
//     // Asserted literals:
//     LIT_STORAGE literals;

//     // Current level:
//     uint32_t level;

//     // Variables used in current trial:
//     VARIABLES variables;
// } TRIAL;

// void trial_init(TRIAL* trial)
// {
//     LIT_STORAGE_init(&trial->literals);
//     trial->level = 0U;

//     variables_init(&trial->variables);
// }

// void trial_free(TRIAL* trial)
// {
//     LIT_STORAGE_free(&trial->literals);
// }

// // Current level for a trial - number of decision literals in it.
// // trial_cur_level(trial) = length(trial_decisions(trial))
// uint32_t trial_cur_level(const TRIAL* trial)
// {
//     return trial->level;
// }

// // Checks whether a given assertion trial unsatisfies a formula:
// bool trial_is_unsat(const TRIAL* trial, const FORMULA* formula)
// {
//     // ToDo
// }

// // Add new literal into a trial:
// void trial_assert_literal(TRIAL* trial, literal_t literal)
// {
//     // Put decision into the literal:
//     LIT_STORAGE_push(&trial->literals, literal);

//     if (literal & LITERAL_DECISION_BIT)
//     {
//         trial->level += 1U;
//     }

//     variables_assert_literal(trial->variables, literal);
// }

// void trial_pop_to_last_decision(TRIAL* trial, literal_t* literal)
// {
//     do
//     {
//         bool ret = LIT_STORAGE_pop(&trial->literals, literal);
//         BUG_ON(!ret, "[trial_pop_to_last_decision] Expected at least one decision literal!")
//     }
//     while (!(*literal & LITERAL_DECISION_BIT));

//     trial->level -= 1U;
// }

// //================//
// // DPLL algorithm //
// //================//

// //
// // Unit propagation
// //
// bool dpll_apply_unit_propagate(TRIAL* trial, const FORMULA* formula)
// {
//     if (
//     /*  Search for unit clause:
//         \exists clause c;
//         \exists literal_t l;
//             c \in formula || clause_is_unit(c, l, trial)*/
//     )
//     {
//         trial_assert_literal(trial, l);
//         return true;
//     }

//     return false;
// }

// void dpll_exhaustive_unit_propagate(TRIAL* trial, const FORMULA* formula)
// {
//     bool ret;
//     do
//     {
//         ret = dpll_apply_unit_propagate(trial, formula);
//     }
//     while (trial_is_unsat(trial, formula) || ret == false);
// }

// //
// // Formula preprocessing
// //

// void dpll_preprocess_formula(const FORMULA* initial, FORMULA* resulting)
// {
//     // ToDo
// }

// //
// // Branching scheme
// //

// literal_t dpll_select_literal(const TRIAL* trial, const FORMULA* formula)
// {
//     // ToDo
// }

// void dpll_apply_decide(TRIAL* trial, const FORMULA* formula)
// {
//     literal_t branching_literal =
//         dpll_select_literal(trial, formula);

//     trial_assert_literal(trial, literal | LITERAL_DECISION_BIT);
// }

// //
// // Backtracking scheme
// //
// void dpll_apply_backtrack(TRIAL* trial)
// {
//     // Pop everything to last decision literal:
//     literal_t last_decision;

//     trial_pop_to_last_decision(trial, &last_decision);

//     // Assert literal with reversed contrarity as non-decision:
//     last_decision ^=  LITERAL_CONTRARY_BIT;
//     last_decision &= ~LITERAL_DECISION_BIT;

//     trial_assert_literal(trial, last_decision);
// }

// //
// // General solver algorithm
// //
// sat_t dpll_solve(const FORMULA* initial_formula)
// {
//     // Perform initial preprocessing for the formula:
//     // NOTE: it is required to initialize invariants
//     //       for the Two Watch Literal Scheme
//     FORMULA* formula;
//     dpll_preprocess_formula(initial_formula, formula);

//     // Satisfiability status:
//     sat_t sat_flag = UNDEF;

//     // Assertion trial:
//     TRIAL trial;
//     trial_init(&trial);

//     while (sat_flag == UNDEF)
//     {
//         // Optimize the search by unit propagation:
//         dpll_exhaustive_unit_propagate(&trial, formula);

//         if (trial_is_unsat(&trial, formula))
//         {
//             if (trial_cur_level(&trial) == 0U)
//             {
//                 // Formula is unsatisfiable with no substitutions => UNSAT.
//                 sat_flag = UNSAT;
//             }
//             else
//             {
//                 // Pop substitution from the trial:
//                 trial_apply_backtrack(&trial);
//             }
//         }
//         else
//         {
//             const VARIABLES* formula_vars = &formula.variables;
//             const VARIABLES*   trial_vars = &trial->variables;

//             if (variables_equal(formula_vars, trial_vars))
//             {
//                 // Explicitly get the valuation that satisfies the formula => SAT.
//                 sat_flag = SAT;
//             }
//             else
//             {
//                 // Use decision to obtain substitution:
//                 dpll_apply_decide(&trial);
//             }
//         }
//     }

//     trial_free(&trial);
// }

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

    FORMULA_print(&to_solve);

    return EXIT_SUCCESS;
}
