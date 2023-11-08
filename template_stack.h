//============================//
// Imitation of C++ templates //
//============================//

// Expect macro _DATA_T_ (data type to be stored in the data structure)
#ifndef _DATA_T_
#error "[ERROR] Expected macro _DATA_T_ to be defined to element type"
#endif

// Expect macro _DATA_STRUCTURE_ (name of the data structure in the program)
#ifndef _DATA_STRUCTURE_
#error "[ERROR] Expected macro _DATA_STRUCTURE_ to be defined to element type"
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "utils.h"

//================//
// Data structure //
//================//

typedef struct
{
    // Array for stack elements:
    _DATA_T_* array;

    // Number of elements stored in an array:
    size_t size;

    // Maximum possible size for allocated chunk of memory (measured in elements):
    size_t capacity;
} _DATA_STRUCTURE_;

//====================//
// Stack verification //
//====================//

// Verify stack internal structure.
// NOTE: this function is for internal use only.
bool _DATA_STRUCTURE__ok(_DATA_STRUCTURE_* stack)
{
    if (stack->capacity != 0 && stack->array == NULL)
    {
        return false;
    }

    if (stack->size > stack->capacity)
    {
        return false;
    }

    return true;
}

//===================//
// Memory management //
//===================//

// Initialize stack to initial known state.
void _DATA_STRUCTURE__init(_DATA_STRUCTURE_* stack)
{
    assert(stack != NULL);

    // Initialize size and capacity:
    stack->size = 0U;
    stack->capacity = 1U;

    // Allocate stack of size one:
    stack->array = malloc(1U * sizeof(_DATA_T_));
    VERIFY_CONTRACT(
        stack->array != NULL,
        "["#_DATA_STRUCTURE__init"] Unable to allocate memory for stack of capacity %zu\n",
        stack->capacity);
}

// Deallocate stack memory:
void _DATA_STRUCTURE__free(_DATA_STRUCTURE_* stack)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        _DATA_STRUCTURE__ok(stack),
        "["#_DATA_STRUCTURE__free"] Unable to free an invalid stack (possible double free)\n");

    free(stack->array);

    // Mark stack array as invalid:
    // So that OK will raise error:
    stack->array    = NULL;
    stack->capacity = 0xAA00AA00U;
    stack->size     = 0xAAAAAAAAU;
}

// Perform a resize operation.
// NOTE: this function is for internal use only.
void _DATA_STRUCTURE__resize(_DATA_STRUCTURE_* stack, size_t new_capacity)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        _DATA_STRUCTURE__ok(stack),
        "["#_DATA_STRUCTURE__resize"] Unable to resize an invalid stack\n");

    // Allocate new array:
    _DATA_T_* new_array = realloc(stack->array, new_capacity * sizeof(_DATA_T_));
    VERIFY_CONTRACT(
        new_capacity == 0U || new_array != NULL,
        "["#_DATA_STRUCTURE__resize"] Unable to reallocate memory for stack of new capacity %zu\n",
        new_capacity);

    // Calculate resulting stack size:
    size_t new_size = MIN(stack->size, new_capacity);

    // Update stack data:
    stack->array    = new_array;
    stack->size     = new_size;
    stack->capacity = new_capacity;
}

//==============//
// Stack access //
//==============//

_DATA_T_ _DATA_STRUCTURE__get(_DATA_STRUCTURE_* stack, size_t index)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        _DATA_STRUCTURE__ok(stack),
        "["#_DATA_STRUCTURE__get"] Unable to get element from invalid stack\n");

    VERIFY_CONTRACT(
        index < stack->size,
        "["#_DATA_STRUCTURE__get"] Access out of bounds (index=%zu, size=%zu)\n",
        index, stack->size);

    return stack->array[index];
}

_DATA_T_* _DATA_STRUCTURE__get_ptr(_DATA_STRUCTURE_* stack, size_t index)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        _DATA_STRUCTURE__ok(stack),
        "["#_DATA_STRUCTURE__get_ptr"] Unable to get element from invalid stack\n");

    VERIFY_CONTRACT(
        index < stack->size,
        "["#_DATA_STRUCTURE__get_ptr"] Access out of bounds (index=%zu, size=%zu)\n",
        index, stack->size);

    return &stack->array[index];
}

//==================//
// Stack operations //
//==================//

// Push an element on top of the stack.
void _DATA_STRUCTURE__push(_DATA_STRUCTURE_* stack, _DATA_T_ element)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        _DATA_STRUCTURE__ok(stack),
        "[stack_push] Unable to push to an invalid stack\n");

    // Resize to provide space for new element:
    if (stack->size == stack->capacity)
    {
        size_t new_capacity =
            (stack->size == 0U)? 4U : (2U * stack->capacity);

        _DATA_STRUCTURE__resize(stack, new_capacity);
    }

    // Push element:
    stack->array[stack->size] = element;

    stack->size += 1U;
}

// Pop an element from the stack.
//
// Return true on success.
// Return false if there are no elements in the stack.
bool _DATA_STRUCTURE__pop(_DATA_STRUCTURE_* stack, _DATA_T_* element)
{
    assert(stack   != NULL);
    assert(element != NULL);

    VERIFY_CONTRACT(
        _DATA_STRUCTURE__ok(stack),
        "[stack_push] Unable to pop from an invalid stack\n");

    // Do not pop from empty stack:
    if (stack->size == 0U)
    {
        return false;
    }

    // Copy element to specified memory location:
    *element = stack->array[stack->size - 1U];

    stack->size -= 1U;

    // Resize array down:
    if (stack->size <= stack->capacity / 8U)
    {
        size_t new_capacity = stack->capacity / 2U;

        _DATA_STRUCTURE__resize(stack, new_capacity);
    }

    return true;
}
