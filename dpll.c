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

//==========================//
// Watch list implemetation //
//==========================//

#define DATA_T         const CLAUSE*
#define DATA_STRUCTURE WATCHED_STORAGE
#include "template_stack.h"

bool CLAUSE_PTR_eq(const CLAUSE** el1, const CLAUSE** el2)
{
    return *el1 == *el2;
}

bool CLAUSE_PTR_lt(const CLAUSE** el1, const CLAUSE** el2)
{
    BUG_ON(false, "[%s] Invalid operation", "CLAUSE_PTR_lt");
}

typedef struct
{
    WATCHED_STORAGE* clause_lists;
    size_t num_literals;
} WATCH_LIST;

void WATCH_LIST_init(WATCH_LIST* wl, size_t num_literals)
{
    wl->num_literals = num_literals;
    wl->clause_lists = calloc(2U*num_literals, sizeof(WATCHED_STORAGE));
    for (size_t i = 0U; i < 2U*num_literals; ++i)
    {
        WATCHED_STORAGE_init(&wl->clause_lists[i],
            CLAUSE_PTR_eq, CLAUSE_PTR_lt, false);
    }
}

WATCHED_STORAGE* WATCH_LIST_get(WATCH_LIST* wl, literal_t lit)
{
    size_t index = LITERAL_VALUE_Get(lit) - 1U;

    if (lit & LITERAL_CONTRARY_BIT)
    {
        index += wl->num_literals;
    }

    return &wl->clause_lists[index];
}

void WATCH_LIST_link_initial(WATCH_LIST* wl, const FORMULA* formula)
{
    for (size_t cls_i = 0U; cls_i < FORMULA_size(formula); ++cls_i)
    {
        CLAUSE* cls = FORMULA_get(formula, cls_i);

        // Add both wathes to watch initial watch lists:
        literal_t watch1 = CLAUSE_watch1(cls);
        literal_t watch2 = CLAUSE_watch2(cls);

        WATCHED_STORAGE* wl1 = WATCH_LIST_get(wl, watch1);
        WATCHED_STORAGE* wl2 = WATCH_LIST_get(wl, watch2);

        if (!WATCHED_STORAGE_find(wl1, cls))
        {
            WATCHED_STORAGE_push(wl1, cls);
        }

        if (!WATCHED_STORAGE_find(wl2, cls))
        {
            WATCHED_STORAGE_push(wl2, cls);
        }
    }
}

void WATCH_LIST_set(WATCH_LIST* wl, literal_t lit, WATCHED_STORAGE ws)
{
    size_t index = LITERAL_VALUE_Get(lit) - 1U;

    if (lit & LITERAL_CONTRARY_BIT)
    {
        index += wl->num_literals;
    }

    WATCHED_STORAGE_free(&wl->clause_lists[index]);

    wl->clause_lists[index] = ws;
}

void WATCH_LIST_free(WATCH_LIST* wl)
{
    for (size_t i = 0U; i < 2U*wl->num_literals; ++i)
    {
        WATCHED_STORAGE_free(&wl->clause_lists[i]);
    }
}

void WATCH_LIST_print(WATCH_LIST* wl, const FORMULA* formula)
{
    for (size_t index = 0U; index < 2U * wl->num_literals; ++index)
    {
        int value = (index % wl->num_literals) + 1;
        if (index >= wl->num_literals)
        {
            value *= -1;
        }

        WATCHED_STORAGE* ws = &wl->clause_lists[index];

        if (ws->size == 0U)
        {
            continue;
        }

        printf("WL[%4d]", value);

        for (size_t cls_i = 0U; cls_i < ws->size; ++cls_i)
        {
            // Fuck with the type system a bit more:
            CLAUSE* cls = (CLAUSE*)(void*) WATCHED_STORAGE_get(ws, cls_i);

            size_t cls_i = cls - formula->clauses.array;

            printf(" %4zu", cls_i);
        }

        printf("\n");
    }
}

//================================//
// Assertion trial data structure //
//================================//

typedef struct {
    // Asserted literals:
    LIT_STORAGE literals;
    LIT_STORAGE assertion_queue;

    // Current level:
    uint32_t level;

    // Variables used in current trial:
    VARIABLES variables;

    // Variables not used in current trial:
    VARIABLES unselected;

    // Flag used to check for unsatisfyibility:
    bool conflict_flag;

    // Watch list:
    WATCH_LIST wl;
} TRIAL;

void TRIAL_init(TRIAL* trial, size_t num_literals)
{
    LIT_STORAGE_init(
        &trial->literals,
        &LITERAL_eq_contrarity,
        &LITERAL_lt,
        false);

    LIT_STORAGE_init(
        &trial->assertion_queue,
        &LITERAL_eq_contrarity,
        &LITERAL_lt,
        false);

    trial->level = 0U;

    VARIABLES_init(&trial->variables);
    VARIABLES_init(&trial->unselected);

    trial->conflict_flag = false;

    WATCH_LIST_init(&trial->wl, num_literals);
}

void TRIAL_free(TRIAL* trial)
{
    LIT_STORAGE_free(&trial->literals);
    LIT_STORAGE_free(&trial->assertion_queue);
    WATCH_LIST_free(&trial->wl);
}

void TRIAL_print(TRIAL* trial)
{
    for (size_t lit_i = 0U; lit_i < trial->literals.size; ++lit_i)
    {
        literal_t lit = LIT_STORAGE_get(&trial->literals, lit_i);

        printf("%5d ", LITERAL_value(lit));
    }

    printf("\n");
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

bool TRIAL_literal_is_undef(const TRIAL* trial, literal_t lit)
{
    return VARIABLES_literal_is_undef(&trial->variables, lit);
}

void TRIAL_add_to_assertion_queue(TRIAL* trial, literal_t literal)
{
    if (!LIT_STORAGE_find(&trial->assertion_queue, literal))
    {
        // Enqueue literal:
        LIT_STORAGE_insert(&trial->assertion_queue, literal, 0U);
    }
}

// Checks whether a given assertion trial unsatisfies a formula:
bool TRIAL_formula_is_unsat(const TRIAL* trial)
{
    printf("[CHECK SAT ] Trial is %s\n", trial->conflict_flag? "UNSAT" : "SAT");

    return trial->conflict_flag;
}

void TRIAL_pop_to_last_decision(TRIAL* trial, literal_t* literal)
{
    bool ret;
    do
    {
        literal_t dumpster;
        ret = LIT_STORAGE_pop(&trial->assertion_queue, &dumpster);
    }
    while (ret);

    do
    {
        bool ret = LIT_STORAGE_pop(&trial->literals, literal);
        BUG_ON(!ret, "[%s] Expected at least one decision literal!", "trial_pop_to_last_decision");

        VARIABLES_remove_literal(&trial->variables,  *literal);
        VARIABLES_assert_literal(&trial->unselected, *literal);
    }
    while (!(*literal & LITERAL_DECISION_BIT));

    trial->level -= 1U;
}

//================//
// DPLL algorithm //
//================//

//
// Debugging utility
//
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define MAGENTA "\033[1;35m"
#define RESET   "\033[0m"

void dpll_print_progress(TRIAL* trial, FORMULA* formula)
{
    TRIAL_print(trial);

    printf("[TO ASSERT ] ");
    for (size_t lit_i = 0U; lit_i < trial->assertion_queue.size; ++lit_i)
    {
        literal_t lit = LIT_STORAGE_get(&trial->assertion_queue, lit_i);

        printf(YELLOW"%5d ", LITERAL_value(lit));
    }
    printf("\n"RESET);

    for (size_t cls_i = 0U; cls_i < FORMULA_size(formula); ++cls_i)
    {
        CLAUSE* cls = FORMULA_get(formula, cls_i);

        printf("[CLAUSE %3zu] ", cls_i);

        for (size_t lit_i = 0U; lit_i < CLAUSE_size(cls); ++lit_i)
        {
            literal_t lit = CLAUSE_get(cls, lit_i);

            const char* color = TRIAL_literal_is_false(trial, lit)? RED    :
                                TRIAL_literal_is_true(trial, lit) ? GREEN  :
                                                                    MAGENTA;

            printf("%s%5d ", color, LITERAL_value(lit));
        }

        printf("\n"RESET);
    }
}

//
// Literal assertion
//

void dpll_notify_watches(TRIAL* trial, FORMULA* formula, literal_t literal)
{
    // NOTE: literal is the inversion of the asserted literal

    // Get current watch list to work with:
    WATCHED_STORAGE* wl = WATCH_LIST_get(&trial->wl, literal);

    // Create a new watch list:
    WATCHED_STORAGE newWL;
    WATCHED_STORAGE_init(&newWL, CLAUSE_PTR_eq, CLAUSE_PTR_lt, false);

    for (size_t cls_i = 0U; cls_i < wl->size; ++cls_i)
    {
        // Fuck with the type system a bit:
        CLAUSE* cls = (CLAUSE*)(void*) WATCHED_STORAGE_get(wl, cls_i);

        // Quick check whether clause contains a true literal:
        if (TRIAL_literal_is_true(trial, CLAUSE_watch1(cls)))
        {
            // True clauses do not need any notifications:
            continue;
        }

        // Check whether a watched literal is falsified:
        if (CLAUSE_watch1(cls) != literal && CLAUSE_watch2(cls) != literal)
        {
            continue;
        }

        // Ensure that second literal is falsified:
        if (CLAUSE_watch1(cls) == literal)
        {
            CLAUSE_swap_watches(cls);
        }

        // At this point:
        // watch1 = UNDEF/FALSE/TRUE
        // watch2 = FALSE
        // Case of  TRUE/FALSE is a true clause => no notification
        if (TRIAL_literal_is_true(trial, CLAUSE_watch1(cls)))
        {
            // Add clause to watch list:
            WATCHED_STORAGE_push(&newWL, cls);

            continue;
        }

        // At this point:
        // watch1 = FALSE/UNDEF
        // watch2 = FALSE

        // Find first non-watched unfalsified literal:
        bool has_unfalsified = false;
        for (size_t lit_i = 2U; lit_i < CLAUSE_size(cls); ++lit_i)
        {
            literal_t lit = CLAUSE_get(cls, lit_i);
            if (!TRIAL_literal_is_false(trial, lit))
            {
                CLAUSE_set_watch2(cls, lit_i);

                // Add clause to watch list:
                WATCHED_STORAGE_push(&newWL, cls);

                has_unfalsified = true;
                break;
            }
        }

        if (has_unfalsified)
        {
            // Set watches to state:
            // watch1 = FALSE/UNDEF
            // watch2 = UNDEF
            continue;
        }

        // At this point the clause state is:
        // watch1 = FALSE/UNDEF
        // watch2 = FALSE
        // other  = FALSE
        if (TRIAL_literal_is_false(trial, CLAUSE_watch1(cls)))
        {
            // Detect a falsified clause:
            trial->conflict_flag = true;

            // Add clause to watch list:
            WATCHED_STORAGE_push(&newWL, cls);
        }
        else
        {
            // Add unit clause to unit-propagation queue:
            TRIAL_add_to_assertion_queue(trial, CLAUSE_watch1(cls));

            // Add clause to watch list:
            WATCHED_STORAGE_push(&newWL, cls);
        }
    }

    // Update watch list:
    WATCH_LIST_set(&trial->wl, literal, newWL);

    printf(YELLOW"[WATCH %d]\n"RESET, LITERAL_value(literal));
    WATCH_LIST_print(&trial->wl, formula);
}

void dpll_assert_literal(TRIAL* trial, FORMULA* formula, literal_t literal)
{
    // Put decision into the literal:
    LIT_STORAGE_push(&trial->literals, literal);

    if (literal & LITERAL_DECISION_BIT)
    {
        trial->level += 1U;
    }

    VARIABLES_assert_literal(&trial->variables,  literal);
    VARIABLES_remove_literal(&trial->unselected, literal);

    // Ignore the decision bit for the notification:
    literal &= ~LITERAL_DECISION_BIT;

    dpll_notify_watches(trial, formula, literal ^ LITERAL_CONTRARY_BIT);

    // printf(YELLOW"[ASSERT %3d] "RESET, LITERAL_value(literal));
    // dpll_print_progress(trial, formula);

    // printf(YELLOW"[NOTIFY %3d] "RESET, -LITERAL_value(literal));
    // dpll_print_progress(trial, formula);
}

//
// Unit propagation
//
bool dpll_apply_unit_propagate(TRIAL* trial, FORMULA* formula)
{
    if (trial->assertion_queue.size != 0U)
    {
        literal_t lit;
        LIT_STORAGE_pop(&trial->assertion_queue, &lit);

        dpll_assert_literal(trial, formula, lit);
        return true;
    }

    return false;
}

void dpll_exhaustive_unit_propagate(TRIAL* trial, FORMULA* formula)
{
    bool ret;
    do
    {
        ret = dpll_apply_unit_propagate(trial, formula);
    }
    while (!TRIAL_formula_is_unsat(trial) && ret != false);
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
        // (Fuck the type system a bit)
        CLAUSE* clause = FORMULA_get(initial, cls_i);

        // Preprocessed clause to be inserted:
        CLAUSE rslt_clause;
        CLAUSE_init(&rslt_clause);

        // Iterate over literals of a clause and copy them to rslt_clause:
        bool insert_clause = true;
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
            if (CLAUSE_find(&rslt_clause, cur))
            {
                // Detect clause tautology:
                if (CLAUSE_find(&rslt_clause, cur ^ LITERAL_CONTRARY_BIT))
                {
                    // Remove tautological clause:
                    insert_clause = false;
                    break;
                }

                // Do not copy duplicate literals:
                continue;
            }

            // Add literal to the clause:
            CLAUSE_insert(&rslt_clause, cur);

            // Also allow it to be the decision literal in the future:
            VARIABLES_assert_literal(&trial->unselected, cur);
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
            dpll_assert_literal(trial, resulting, CLAUSE_get(&rslt_clause, 0U));
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

    // Initialize watch list:
    WATCH_LIST_link_initial(&trial->wl, resulting);

    return UNDEF;
}

//
// Branching scheme
//

literal_t dpll_select_literal(TRIAL* trial, const FORMULA* formula)
{
    // Iterate over the formula from least clauses to bigger:
    // literal_t selected = LITERAL_NULL;
    // for (size_t cls_i = 0U; cls_i < FORMULA_size(formula); ++cls_i)
    // {
    //     CLAUSE* cls = FORMULA_get(formula, cls_i);

    //     // Check only the watches:
    //     literal_t watch1 = CLAUSE_watch1(cls);
    //     literal_t watch2 = CLAUSE_watch2(cls);

    //     if (TRIAL_literal_is_undef(trial, watch1))
    //     {
    //         selected = watch1 ^ LITERAL_CONTRARY_BIT;
    //         break;
    //     }

    //     if (TRIAL_literal_is_undef(trial, watch2))
    //     {
    //         selected = watch2 ^ LITERAL_CONTRARY_BIT;
    //         break;
    //     }
    // }

    // if (selected != LITERAL_NULL)
    // {
    //     VARIABLES_remove_literal(&trial->unselected, selected);
    //     return selected;
    // }

    return VARIABLES_pop_asserted(&trial->unselected);
}

void dpll_apply_decide(TRIAL* trial, FORMULA* formula)
{
    literal_t branching_literal =
        dpll_select_literal(trial, formula);

    BUG_ON(branching_literal == LITERAL_NULL,
        "[%s] Termination not detected\n", "dpll_apply_decide");

    dpll_assert_literal(trial, formula, branching_literal | LITERAL_DECISION_BIT);
}

//
// Backtracking scheme
//
void dpll_apply_backtrack(TRIAL* trial, FORMULA* formula)
{
    // Pop everything to last decision literal:
    literal_t last_decision;

    TRIAL_pop_to_last_decision(trial, &last_decision);

    // Hopefully eliminate conflict:
    trial->conflict_flag = false;

    // Assert literal with reversed contrarity as non-decision:
    last_decision ^=  LITERAL_CONTRARY_BIT;
    last_decision &= ~LITERAL_DECISION_BIT;

    dpll_assert_literal(trial, formula, last_decision);
}

//
// General solver algorithm
//
sat_t dpll_solve(const FORMULA* initial_formula)
{
    // Assertion trial:
    TRIAL trial;
    TRIAL_init(&trial, initial_formula->variables.num_literals);

    // Satisfiability status:
    sat_t sat_flag = UNDEF;

    // Perform initial preprocessing for the formula:
    // NOTE: it is required to initialize invariants
    //       for the Two Watch Literal Scheme
    FORMULA formula;

    sat_flag = dpll_preprocess_formula(initial_formula, &formula, &trial);

    printf(YELLOW"[PREPROCESS] "RESET);
    dpll_print_progress(&trial, &formula);

    // DPLL algorithm:
    while (sat_flag == UNDEF)
    {
        // Optimize the search by unit propagation:
        dpll_exhaustive_unit_propagate(&trial, &formula);

        printf(YELLOW"[PROPAGATE ] "RESET);
        dpll_print_progress(&trial, &formula);

        if (TRIAL_formula_is_unsat(&trial))
        {
            if (TRIAL_cur_level(&trial) == 0U)
            {
                // Formula is unsatisfiable with no substitutions => UNSAT.
                sat_flag = UNSAT;
            }
            else
            {
                // Pop substitution from the trial:
                dpll_apply_backtrack(&trial, &formula);

                printf(YELLOW"[BACKTRACK ] "RESET);
                TRIAL_print(&trial);
            }
        }
        else
        {
            const VARIABLES* formula_vars = &formula.variables;
            const VARIABLES*   trial_vars = &trial.variables;

            if (VARIABLES_equal(formula_vars, trial_vars))
            {
                // Explicitly get the valuation that satisfies the formula => SAT.
                sat_flag = SAT;
            }
            else
            {
                // Use decision to obtain substitution:
                dpll_apply_decide(&trial, &formula);

                printf(YELLOW"[DECIDE    ] "RESET);
                TRIAL_print(&trial);
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

    printf("%s\n", ret == SAT? "SAT" : "UNSAT");

    FORMULA_free(&to_solve);

    return EXIT_SUCCESS;
}
