// No copyright. Vladislav Aleinik, 2023
#ifndef DPLL_DIMACS_PARSER_H
#define DPLL_DIMACS_PARSER_H

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "utils.h"
#include "formula.h"

#define MAX_LINE_LENGTH 120U

void DIMACS_load_formula(const char* filename, FORMULA* formula)
{
    // Initialize formula:
    FORMULA_init(formula);

    // Open file:
    FILE* cnf_file = fopen(filename, "r");
    VERIFY_CONTRACT(cnf_file != NULL,
        "[DIMACS_load_formula] Unable to open file %s\n", filename);

    // Problem information:
    bool entered_problem = false;
    unsigned num_variables = 0U;
    unsigned num_clauses = 0U;

    // NOTE: +1 is for \n and +1 is for null-terminator.
    char line[MAX_LINE_LENGTH + 2U];

    // Read the whole file:
    unsigned clause_i = 0U;
    for (size_t line_i = 0U; true; line_i++)
    {
        // Read input line:
        char* ret = fgets(line, MAX_LINE_LENGTH + 1U, cnf_file);
        if (ret == NULL)
        {
            break;
        }

        // Get line length:
        size_t len = strlen(line);
        VERIFY_CONTRACT(len > 0 && line[len - 1U] == '\n',
            "[DIMACS_load_formula] Line %zu is too long (expected up to %u characters)\n",
            line_i, MAX_LINE_LENGTH);

        // Parse comment lines:
        if (line[0] == 'c')
        {
            // Do nothing.
        }
        // Parse file termination:
        else if (line[0] == '%')
        {
            break;
        }
        // Parse problem:
        else if (line[0] == 'p')
        {
            VERIFY_CONTRACT(entered_problem == false,
                "[DIMACS_load_formula] Line %zu is a duplicate problem line\n", line_i);

            char dumpster[10U];
            int ret = sscanf(line, "p cnf %u%10[ ]%u",
                &num_variables, dumpster, &num_clauses);
            VERIFY_CONTRACT(ret == 3,
                "[DIMACS_load_formula] Line %zu has invalid format\n", line_i);

            entered_problem = true;
        }
        // Parse clause:
        else
        {
            CLAUSE new_clause;
            CLAUSE_init(&new_clause);

            char* cur = line;
            // Handle spurious leading whitespace:
            while (*cur != '\0' && isspace(*cur))
            {
                cur += 1U;
            }

            while (true)
            {
                // Read a value from line:
                char* endptr = NULL;
                int value = strtol(cur, &endptr, 10U);
                VERIFY_CONTRACT(cur != endptr,
                    "[DIMACS_load_formula] Unable to read clause on line %zu\n",
                    line_i);
                VERIFY_CONTRACT(abs(value) < NUM_LITERALS,
                    "[DIMACS_load_formula] Solver supports literals up to %d (got %u)",
                    NUM_LITERALS, abs(value));

                // Ensure progress:
                cur = endptr;

                if (value == 0)
                {
                    break;
                }

                // Construct literal:
                literal_t lit = 0U;
                lit |= (value < 0)? LITERAL_CONTRARY_BIT : 0U;
                LITERAL_VALUE_Set(lit, abs(value));

                // Insert literal into clause:
                CLAUSE_insert(&new_clause, lit);
            }

            // Insert clause into formula:
            FORMULA_insert(formula, new_clause);

            clause_i += 1U;
        }
    }

    VERIFY_CONTRACT(entered_problem == true,
        "[DIMACS_load_formula] File %s has no problem line\n", filename);

    VERIFY_CONTRACT(clause_i == num_clauses,
        "[DIMACS_load_formula] File %s has mismatched number of clauses (expected %d, got %d)\n",
        filename, num_clauses, clause_i);

    fclose(cnf_file);
}


#endif // DPLL_DIMACS_PARSER_H
