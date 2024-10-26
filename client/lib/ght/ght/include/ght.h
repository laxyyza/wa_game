
#ifndef _GHT_H_
#define _GHT_H_

/*
 * GHT - Generic Hash Table
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef _GHT_THREAD_SAFE
#include <pthread.h>
#endif // _GHT_THREAD_SAFE

typedef void (*ght_free_t)(void* data);

typedef struct ght_bucket
{
    uint64_t    key;
    void*       data;
    int8_t      inheap;

    struct ght_bucket* next;
} ght_bucket_t;

/*
 * Generic Hash Table - (GHT)
 *
 * Featurs:
 *  - Dynamic sizing based on load factor, with adjustable max & min thresholds.
 *  - Thread-Safe.
 *  - Automatic memory management (if `ght_t::free` is provided).
 *  - Generic Types.
 *
 * NOTE:
 *  If `ght_t::free` is provided, GHT will assume ownership of elements.
 */
typedef struct
{
    /* Array */
    ght_bucket_t*   table;
    size_t          size;
    size_t          min_size;
    size_t          count;
    bool            ignore_resize;

#ifdef _GHT_THREAD_SAFE
    pthread_mutex_t mutex;
#endif
    ght_free_t      free;

    /* Load Factor & min/max thresholds */
    float             load;
    float             max_load;
    float             min_load;
} ght_t;

/* return: false if failed */
bool        ght_init(ght_t* ht, size_t initial_size, ght_free_t free_callback);

/* Hash String */
uint64_t    ght_hashstr(const char* str);

/* return: false if `key` already exists in table. */
bool        ght_insert(ght_t* ht, uint64_t key, void* data);

/* return: NULL if not found */
void*       ght_get(ght_t* ht, uint64_t key);

/* return: false if not found. */
bool        ght_del(ght_t* ht, uint64_t key);

/* Delete all elements; clear table. */
void        ght_clear(ght_t* ht);

/* Delete all elements, mutex and table array. */
void        ght_destroy(ght_t* ht);

/* Loop each element in hash table. */
#define GHT_FOREACH(item, ht, code_block)\
    for (size_t __i = 0; __i < ht->size; __i++)\
    {\
        ght_bucket_t* _bucket = ht->table + __i;\
        while (_bucket && _bucket->data)\
        {\
            item = _bucket->data;\
            code_block\
            _bucket = _bucket->next;\
        }\
    }

#ifdef _GHT_THREAD_SAFE
void ght_lock_f(ght_t* ht);
void ght_unlock_f(ght_t* ht);

#define ght_lock(ht) ght_lock_f(ht)
#define ght_unlock(ht) ght_unlock_f(ht)
#else
#define ght_lock(ht) 
#define ght_unlock(ht) 
#endif // _GHT_THREAD_SAFE

#endif // _GHT_H_
