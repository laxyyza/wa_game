#ifndef _ARRAY_H_
#define _ARRAY_H_

#include "int.h"

typedef struct array array_t;
typedef void (*array_resize_callback_t)(array_t* array);

typedef struct array
{
    u8* buf;        // Pointer to start of array
    u32 size;       // Array Size in bytes
    u32 min_size;
    u32 count;      // Element Count
    u32 ele_size;   // Element Size

    array_resize_callback_t resize_callback;
    void* user_data;
    bool changed;
} array_t;

void  array_init(array_t* array, u32 element_size, u32 init_size);
void* array_idx(const array_t* array, u32 idx);
void  array_add(array_t* array, void* element);
void  array_add_i16(array_t* array, u16 element);
void  array_set_resize_cb(array_t* array, array_resize_callback_t cb, 
                          void* user_data);
/**
 *  Instead of adding an element by copying it into 
 *  the array buffer, you get the pointer to the location 
 *  and the array will increase. 
 *  No need to do anoter copy, you can just directly use it.
 */
void* array_add_into(array_t* array);
void  array_clear(array_t* array, bool resize);
void  array_del(array_t* array);
void  array_pop(array_t* array);

#endif // _ARRAY_H_
