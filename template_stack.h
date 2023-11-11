//============================//
// Imitation of C++ templates //
//============================//

// Expect macro DATA_T (data type to be stored in the data structure)
#ifndef DATA_T
#error "[ERROR] Expected macro DATA_T to be defined to element type"
#endif

// Expect macro DATA_STRUCTURE (name of the data structure in the program)
#ifndef DATA_STRUCTURE
#error "[ERROR] Expected macro DATA_STRUCTURE to be defined to element type"
#endif

// Macros to be simplify code generation:
#define        CALL(structure, method) structure ## _ ## method
#define EXPAND_CALL(structure, method) CALL(structure, method)
#define METHOD(method) EXPAND_CALL(DATA_STRUCTURE, method)

#define METHOD_STR(method) STR(METHOD(method))

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
    DATA_T* array;

    // Number of elements stored in an array:
    size_t size;

    // Maximum possible size for allocated chunk of memory (measured in elements):
    size_t capacity;

    // Comparators:
    bool (*comp_eq)(DATA_T* el1, DATA_T* el2);
    bool (*comp_lt)(DATA_T* el1, DATA_T* el2);

    // Sortedness:
    bool sorted;
} DATA_STRUCTURE;

//====================//
// Stack verification //
//====================//

// Verify stack internal structure.
// NOTE: this function is for internal use only.
bool METHOD(ok)(DATA_STRUCTURE* stack)
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
void METHOD(init)(
    DATA_STRUCTURE* stack,
    bool (*comp_eq)(DATA_T* el1, DATA_T* el2),
    bool (*comp_lt)(DATA_T* el1, DATA_T* el2),
    bool sorted)
{
    assert(stack != NULL);

    // Initialize size and capacity:
    stack->size = 0U;
    stack->capacity = 1U;

    // Allocate stack of size one:
    stack->array = malloc(1U * sizeof(DATA_T));
    VERIFY_CONTRACT(
        stack->array != NULL,
        "[%s] Unable to allocate memory for stack of capacity %zu\n",
        METHOD_STR(init),
        stack->capacity);

    // Set comparators:
    stack->comp_eq = comp_eq;
    stack->comp_lt = comp_lt;

    // Set sortedness:
    stack->sorted = sorted;
}

// Deallocate stack memory:
void METHOD(free)(DATA_STRUCTURE* stack)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        METHOD(ok)(stack),
        "[%s] Unable to free an invalid stack (possible double free)\n",
        METHOD_STR(free));

    free(stack->array);

    // Mark stack array as invalid:
    // So that OK will raise error:
    stack->array    = NULL;
    stack->capacity = 0xAA00AA00U;
    stack->size     = 0xAAAAAAAAU;
}

#define RESIZE_ONEUP   1U
#define RESIZE_ONEDOWN 0U
// Perform a resize operation.
// NOTE: this function is for internal use only.
void METHOD(resize)(DATA_STRUCTURE* stack, bool upscale)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        METHOD(ok)(stack),
        "[%s] Unable to resize an invalid stack\n",
        METHOD_STR(resize));

    // Determine new size:
    size_t new_size = stack->size + (upscale? +1U : -1U);

    // Determine new capacity:
    size_t new_capacity = 0U;
    if (upscale && new_size > stack->capacity)
    {
        new_capacity = (stack->size == 0U)? 4U : (2U * stack->capacity);
    }
    else if (!upscale && new_size < stack->capacity / 8U)
    {
        new_capacity = stack->capacity / 2U;
    }
    else
    {
        stack->size = new_size;
        return;
    }

    // Allocate new array:
    DATA_T* new_array = realloc(stack->array, new_capacity * sizeof(DATA_T));
    VERIFY_CONTRACT(
        new_capacity == 0U || new_array != NULL,
        "[%s] Unable to reallocate memory for stack of new capacity %zu\n",
        METHOD_STR(resize),
        new_capacity);

    // Update stack data:
    stack->array    = new_array;
    stack->size     = new_size;
    stack->capacity = new_capacity;
}

//==============//
// Stack access //
//==============//

DATA_T METHOD(get)(DATA_STRUCTURE* stack, size_t index)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        METHOD(ok)(stack),
        "[%s] Unable to get element from invalid stack\n",
        METHOD_STR(get));

    VERIFY_CONTRACT(
        index < stack->size,
        "[%s] Access out of bounds (index=%zu, size=%zu)\n",
        METHOD_STR(get),
        index, stack->size);

    return stack->array[index];
}

DATA_T* METHOD(get_ptr)(DATA_STRUCTURE* stack, size_t index)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        METHOD(ok)(stack),
        "[%s] Unable to get element from invalid stack\n",
        METHOD_STR(get_ptr));

    VERIFY_CONTRACT(
        index < stack->size,
        "[%s] Access out of bounds (index=%zu, size=%zu)\n",
        METHOD_STR(get_ptr),
        index, stack->size);

    return &stack->array[index];
}

// Find an upper bound for an element.
//
// For internal use only.
size_t METHOD(ubound_sorted)(DATA_STRUCTURE* stack, DATA_T element)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(METHOD(ok)(stack),
        "[%s] Unable to get element from invalid stack\n",
        METHOD_STR(ubound_sorted));

    VERIFY_CONTRACT(stack->sorted,
        "[%s] Unable to search in non-sorted array\n",
        METHOD_STR(ubound_sorted));

    // Upper bound from ACSL-by-Example:
    size_t left = 0U, right = stack->size;
    while (left < right)
    {
        size_t mid = left + (right - left)/2U;
        if (!stack->comp_lt(&element, &stack->array[mid]))
        {
            left = mid + 1U;
        }
        else
        {
            right = mid;
        }
    }

    return right;
}

// Search for an element:
size_t METHOD(search_sorted)(DATA_STRUCTURE* stack, DATA_T element)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(METHOD(ok)(stack),
        "[%s] Unable to get search in invalid stack\n",
        METHOD_STR(search_sorted));

    VERIFY_CONTRACT(stack->sorted,
        "[%s] Unable to search in non-sorted array\n",
        METHOD_STR(search_sorted));

    // Lower bound from ACSL-by-Example:
    size_t left = 0U, right = stack->size;
    while (left < right)
    {
        size_t mid = left + (right - left)/2U;
        if (stack->comp_lt(&stack->array[mid], &element))
        {
            left = mid + 1U;
        }
        else
        {
            right = mid;
        }
    }

    return left;
}

bool METHOD(find_sorted)(DATA_STRUCTURE* stack, DATA_T element)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(METHOD(ok)(stack),
        "[%s] Unable to get find element in invalid stack\n",
        METHOD_STR(find_sorted));

    VERIFY_CONTRACT(stack->sorted,
        "[%s] Unable to search in non-sorted array\n",
        METHOD_STR(find_sorted));

    size_t index = METHOD(search_sorted)(stack, element);

    return (index < stack->size) && stack->comp_eq(&stack->array[index], &element);
}

//==================//
// Stack operations //
//==================//

// Push an element on top of the stack.
void METHOD(push)(DATA_STRUCTURE* stack, DATA_T element)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        METHOD(ok)(stack),
        "[%s] Unable to push to an invalid stack\n",
        METHOD_STR(push));

    VERIFY_CONTRACT(!stack->sorted,
        "[%s] Unable to push to sorted array\n",
        METHOD_STR(push));

    // Resize to provide space for new element:
    METHOD(resize)(stack, RESIZE_ONEUP);

    // Push element:
    stack->array[stack->size - 1U] = element;
}

// Pop an element from the stack.
//
// Return true on success.
// Return false if there are no elements in the stack.
bool METHOD(pop)(DATA_STRUCTURE* stack, DATA_T* element)
{
    assert(stack   != NULL);
    assert(element != NULL);

    VERIFY_CONTRACT(
        METHOD(ok)(stack),
        "[%s] Unable to pop from an invalid stack\n",
        METHOD_STR(pop));

    // Do not pop from empty stack:
    if (stack->size == 0U)
    {
        return false;
    }

    // Copy element to specified memory location:
    *element = stack->array[stack->size - 1U];

    METHOD(resize)(stack, RESIZE_ONEDOWN);

    return true;
}

// Insert an element into stack
void METHOD(insert)(DATA_STRUCTURE* stack, DATA_T element, size_t index)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        METHOD(ok)(stack),
        "[%s] Unable to insert element into invalid stack\n",
        METHOD_STR(insert));
    VERIFY_CONTRACT(
        index <= stack->size,
        "[%s] Index out of bounds\n",
        METHOD_STR(insert));

    VERIFY_CONTRACT(!stack->sorted,
        "[%s] Unable to insert into sorted array\n",
        METHOD_STR(insert));

    // Resize to provide space for new element:
    METHOD(resize)(stack, RESIZE_ONEUP);

    // Move element one right:
    for (size_t copy_i = stack->size; index < copy_i; copy_i--)
    {
        stack->array[copy_i] = stack->array[copy_i - 1U];
    }

    stack->array[index] = element;
}

void METHOD(insert_sorted)(DATA_STRUCTURE* stack, DATA_T element)
{
    assert(stack != NULL);

    VERIFY_CONTRACT(
        METHOD(ok)(stack),
        "[%s] Unable to insert element into invalid stack\n",
        METHOD_STR(insert_sorted));

    VERIFY_CONTRACT(stack->sorted,
        "[%s] Unable to insert into non-sorted array\n",
        METHOD_STR(insert_sorted));

    // Resize to provide space for new element:
    METHOD(resize)(stack, RESIZE_ONEUP);

    size_t index = METHOD(ubound_sorted)(stack, element);

    // Move element one right:
    for (size_t copy_i = stack->size; index < copy_i; copy_i--)
    {
        stack->array[copy_i] = stack->array[copy_i - 1U];
    }

    stack->array[index] = element;
}

// Undefine macros to other uses of this
#undef DATA_T
#undef DATA_STRUCTURE
