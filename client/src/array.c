#include "array.h"
#include <stdlib.h>
#include <string.h>

void  
array_init(array_t* array, u32 element_size, u32 init_size)
{
    array->size = element_size * init_size;
    array->min_size = array->size;
    array->ele_size = element_size;
    array->count = 0;
    array->buf = calloc(element_size, init_size);
    array->resize_callback = NULL;
    array->changed = false;
}

void* 
array_idx(const array_t* array, u32 idx)
{
    return array->buf + (array->ele_size * idx);
}

static inline u64
used_bytes(const array_t* array)
{
    return array->count * array->ele_size;
}

static void
array_resize(array_t* array, u64 new_size)
{
    u64 used_size;
    if (new_size < array->min_size)
        new_size = array->min_size;

    /**
     *  We don't want to cut off used elements.
     *  If new_size is less than used_size
     *  then set new_size to used_size + ele_size.
     *  to have room for at least one element.
     */
    if ((used_size = used_bytes(array)) > new_size)
        new_size = used_size + array->ele_size;

    if (new_size == array->size)
        return;

    array->size = new_size;
    array->buf = realloc(array->buf, new_size);

    if (array->resize_callback)
        array->resize_callback(array);
}

static void
array_inc(array_t* array)
{
    array->count++;
    array->changed = true;

    if ((array->count * array->ele_size) >= array->size)
        array_resize(array, array->size + array->min_size);
}

void  
array_add(array_t* array, void* element)
{
    const u32 offset = array->ele_size * array->count;
    memcpy(array->buf + offset, element, array->ele_size);
    array_inc(array);
}

void
array_add_i16(array_t* array, u16 element)
{
    u16* buf = (u16*)array->buf;
    buf[array->count] = element;
    array_inc(array);
}

void* 
array_add_into(array_t* array)
{
    u32 idx = array->count;

    array_inc(array);

    return array_idx(array, idx);
}

void  
array_clear(array_t* array, bool resize)
{
    array->count = 0;
    array->changed = true;
    if (resize && array->size != array->min_size)
        array_resize(array, array->min_size);
}

void  
array_del(array_t* array)
{
    free(array->buf);
}

void  
array_set_resize_cb(array_t* array, array_resize_callback_t cb, void* user_data)
{
    array->resize_callback = cb;
    array->user_data = user_data;
}

void 
array_pop(array_t* array)
{
    u64 size_left;
    if (array->count == 0)
        return;
    array->count--;
    array->changed = true;

    size_left = array->size - used_bytes(array);
    if (size_left > array->min_size)
        array_resize(array, array->size - array->min_size);
}
