// resman v0.1
//
// MIT License
//
// Copyright (c) 2023 Amil Khisamov
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef RESMAN_H
#define RESMAN_H

#include <stdlib.h>
#include <string.h>

#ifndef RESMAN_ASSERT
#include <assert.h>
#define RESMAN_ASSERT(x) assert(x)
#endif // RESMAN_ASSERT

#ifndef RESMAN_MALLOC
#define RESMAN_MALLOC(x) malloc(x)
#endif // RESMAN_MALLOC

#ifndef RESMAN_CALLOC
#define RESMAN_CALLOC(x, y) calloc(x, y)
#endif // RESMAN_CALLOC

#ifndef RESMAN_FREE
#define RESMAN_FREE(x) free(x)
#endif // RESMAN_FREE

#ifndef RESMAN_HT_INITIAL_CAPACITY
#define RESMAN_HT_INITIAL_CAPACITY 16
#endif // RESMAN_HT_INITIAL_CAPACITY

typedef struct resman_storage_s resman_storage;
typedef void resman_resource;
typedef resman_resource* (*resman_resource_factory_fptr)(const char* path);

static resman_storage* resman_create(const char* base_dir);
static void resman_destroy(resman_storage** storage);
static void resman_register_factory(resman_storage* storage,
    const char* type_name, resman_resource_factory_fptr factory);
static resman_resource* resman_load(const resman_storage* storage,
    const char* type_name, const char* path);
static void resman_unload(const resman_storage* storage, const char* type_name,
    resman_resource** resource);
static int resman_has(const resman_storage* storage, const char* type_name,
    const char* path);

#define RESMAN_REGISTER_FACTORY(storage, type, factory) resman_register_factory(storage, #type, factory)
#define RESMAN_LOAD(storage, type, path) resman_load(storage, #type, path)
#define RESMAN_UNLOAD(storage, type, resource) resman_unload(storage, #type, (resman_resource*) resource)
#define RESMAN_HAS(storage, type, path) resman_has(storage, #type, path)

#endif // RESMAN_H

#ifdef RESMAN_IMPLEMENTATION

#define RESMAN_FNV_OFFSET 14695981039346656037UL
#define RESMAN_FNV_PRIME 1099511628211UL

// HASH TABLE DEFINITION
typedef struct resman_ht_s resman_ht;
typedef struct resman_ht_entry_s resman_ht_entry;

resman_ht* _resman_ht_create();
void _resman_ht_free(resman_ht** table);
void* _resman_ht_get(resman_ht* table, const char* key);
int _resman_ht_has(resman_ht* table, const char* key);
const char* _resman_ht_set(resman_ht* table, const char* key, void* value);
void _resman_ht_erase(resman_ht* table, const char* key);
void _resman_ht_erase_by_value(resman_ht* table, void* value);
// HASH TABLE DEFINITION

// RESMAN

struct resman_storage_s
{
    char* base_dir;
    resman_ht* caches; // (const char* type_name, resman_cache* cache)
};

typedef struct resman_cache_s
{
    resman_resource_factory_fptr factory;
    resman_ht* data; // (const char* path, resman_resource* resource)
} resman_cache;

static resman_storage* resman_create(const char* base_dir)
{
    resman_storage* storage = (resman_storage*)RESMAN_MALLOC(sizeof(resman_storage));
    if (storage == NULL)
    {
        return NULL;
    }
    storage->base_dir = (char*) RESMAN_MALLOC(strlen(base_dir) + 1);
    if (storage->base_dir == NULL)
    {
        RESMAN_FREE(storage);
        return NULL;
    }
    storage->caches = _resman_ht_create();
    strcpy(storage->base_dir, base_dir);

    return storage;
}

static void resman_destroy(resman_storage** storage)
{
    if (storage == NULL)
    {
        return;
    }

    _resman_ht_free(&(*storage)->caches);
    RESMAN_FREE((*storage)->base_dir);
    RESMAN_FREE(*storage);
    *storage = NULL;
}

static resman_resource* resman_load(const resman_storage* storage,
    const char* type_name, const char* path)
{
    if (storage == NULL)
    {
        return NULL;
    }
    resman_cache* cache = (resman_cache*)_resman_ht_get(storage->caches,
        type_name);
    if (cache == NULL)
    {
        return NULL;
    }
    resman_resource* resource = (resman_resource*)_resman_ht_get(cache->data,
        path);
    if (resource == NULL)
    {
        if (cache->factory == NULL)
        {
            return NULL;
        }
        resource = cache->factory(path);
        if (resource == NULL)
        {
            return NULL;
        }
        _resman_ht_set(cache->data, path, resource);
    }
    return resource;
}

static void resman_unload(const resman_storage* storage,
    const char* type_name, resman_resource** resource)
{
    if (storage == NULL || resource == NULL)
    {
        return;
    }
    resman_cache* cache = (resman_cache*)_resman_ht_get(storage->caches,
        type_name);
    if (cache == NULL)
    {
        return;
    }

    _resman_ht_erase_by_value(cache->data, *resource);
    *resource = NULL;
}

static int resman_has(const resman_storage* storage, const char* type_name,
    const char* path)
{
    resman_cache* cache = (resman_cache*)_resman_ht_get(storage->caches,
        type_name);
    if (cache == NULL)
    {
        return 0;
    }
    return _resman_ht_has(cache->data, path);
}

static void resman_register_factory(resman_storage* storage,
    const char* type_name, resman_resource_factory_fptr factory)
{
    if (storage == NULL)
    {
        return;
    }
    resman_cache* cache = (resman_cache*)_resman_ht_get(storage->caches,
        type_name);
    if (cache == NULL)
    {
        cache = (resman_cache*) RESMAN_MALLOC(sizeof(resman_cache));
        if (cache == NULL)
        {
            return;
        }
        cache->factory = factory;
        cache->data = _resman_ht_create();
        _resman_ht_set(storage->caches, type_name, cache);
    }
    else
    {
        cache->factory = factory;
    }
}
// RESMAN

// HASH TABLE IMPLEMENTATION
struct resman_ht_entry_s
{
    char* key;
    void* value;
};

struct resman_ht_s
{
    resman_ht_entry* entries;
    size_t capacity;
    size_t length;
};

unsigned long _resman_hash_key(const char* key)
{
    unsigned long hash = RESMAN_FNV_OFFSET;
    int i;
    for (i = 0; i < strlen(key); ++i)
    {
        hash ^= (unsigned long)(unsigned char)key[i];
        hash *= RESMAN_FNV_PRIME;
    }
    return hash;
}

const char* _resman_ht_set_entry(resman_ht_entry* entries, size_t capacity,
    const char* key, void* value, size_t* plength)
{
    unsigned long hash = _resman_hash_key(key);
    size_t index = (size_t)(hash & (unsigned long)(capacity - 1));

    while (entries[index].key != NULL)
    {
        if (strcmp(key, entries[index].key) == 0)
        {
            entries[index].value = value;
            return key;
        }

        index++;
        if (index >= capacity)
        {
            index = 0;
        }
    }

    if (plength != NULL)
    {
        (*plength)++;
    }

    entries[index].key = (char*) RESMAN_MALLOC(strlen(key) + 1);
    entries[index].value = value;
    strcpy(entries[index].key, key);
    return key;
}

void _resman_ht_erase_entry(resman_ht_entry* entries, size_t capacity,
    const char* key, size_t* plength)
{
    unsigned long hash = _resman_hash_key(key);
    size_t index = (size_t)(hash & (unsigned long)(capacity - 1));

    while (entries[index].key != NULL)
    {
        if (strcmp(key, entries[index].key) == 0)
        {
            RESMAN_FREE(entries[index].key);
            RESMAN_FREE(entries[index].value);
            entries[index].key = NULL;
            entries[index].value = NULL;
            if (plength != NULL)
            {
                (*plength)--;
            }
            return;
        }

        index++;
        if (index >= capacity)
        {
            index = 0;
        }
    }
}

void _resman_ht_erase_entry_by_value(resman_ht_entry* entries, size_t capacity,
    void* value, size_t* plength)
{
    RESMAN_ASSERT(value != NULL);
    if (value == NULL)
    {
        return;
    }

    size_t i;
    for (i = 0; i < capacity; ++i)
    {
        if (entries[i].value == value)
        {
            RESMAN_FREE(entries[i].key);
            RESMAN_FREE(entries[i].value);
            entries[i].key = NULL;
            entries[i].value = NULL;
            if (plength != NULL)
            {
                (*plength)--;
            }
            return;
        }
    }
}

int _resman_ht_expand(resman_ht* table)
{
    size_t new_capacity = table->capacity * 2;
    if (new_capacity < table->capacity)
    {
        return 0;
    }
    resman_ht_entry* new_entries = RESMAN_CALLOC(new_capacity,
        sizeof(resman_ht_entry));
    if (new_entries == NULL)
    {
        return 0;
    }

    size_t i;
    for (i = 0; i < table->capacity; ++i)
    {
        resman_ht_entry entry = table->entries[i];
        if (entry.key != NULL)
        {
            _resman_ht_set_entry(new_entries, new_capacity, entry.key,
                entry.value, NULL);
        }
    }

    RESMAN_FREE(table->entries);
    table->entries = new_entries;
    table->capacity = new_capacity;
    return 1;
}

resman_ht* _resman_ht_create()
{
    resman_ht* table = RESMAN_MALLOC(sizeof(resman_ht));
    if (table == NULL)
    {
        return NULL;
    }

    table->length = 0;
    table->capacity = RESMAN_HT_INITIAL_CAPACITY;

    table->entries = RESMAN_CALLOC(table->capacity, sizeof(resman_ht_entry));
    if (table->entries == NULL)
    {
        RESMAN_FREE(table);
        return NULL;
    }
    return table;
}

void _resman_ht_free(resman_ht** table)
{
    size_t i;
    for (i = 0; i < (*table)->capacity; ++i)
    {
        RESMAN_FREE((*table)->entries[i].key);
        resman_cache* cache = (resman_cache*)(*table)->entries[i].value;
        if (cache != NULL)
        {
            _resman_ht_free(&cache->data);
        }
        RESMAN_FREE((*table)->entries[i].value);
    }

    RESMAN_FREE((*table)->entries);
    RESMAN_FREE(*table);
    *table = NULL;
}

void* _resman_ht_get(resman_ht* table, const char* key)
{
    unsigned long hash = _resman_hash_key(key);
    size_t index = (size_t)(hash & (unsigned long)(table->capacity - 1));

    while (table->entries[index].key != NULL)
    {
        if (strcmp(key, table->entries[index].key) == 0)
        {
            return table->entries[index].value;
        }
        index++;
        if (index >= table->capacity)
        {
            index = 0;
        }
    }
    return NULL;
}

int _resman_ht_has(resman_ht* table, const char* key)
{
    unsigned long hash = _resman_hash_key(key);
    size_t index = (size_t)(hash & (unsigned long)(table->capacity - 1));

    while (table->entries[index].key != NULL)
    {
        if (strcmp(key, table->entries[index].key) == 0)
        {
            return 1;
        }
        index++;
        if (index >= table->capacity)
        {
            index = 0;
        }
    }
    return 0;
}

const char* _resman_ht_set(resman_ht* table, const char* key, void* value)
{
    RESMAN_ASSERT(value != NULL);
    if (value == NULL)
    {
        return NULL;
    }

    if (table->length >= table->capacity / 2)
    {
        if (_resman_ht_expand(table) == 0)
        {
            return NULL;
        }
    }

    return _resman_ht_set_entry(table->entries, table->capacity,
        key, value, &table->length);
}

void _resman_ht_erase(resman_ht* table, const char* key)
{
    _resman_ht_erase_entry(table->entries, table->capacity, key,
        &table->length);
}

void _resman_ht_erase_by_value(resman_ht* table, void* value)
{
    _resman_ht_erase_entry_by_value(table->entries, table->capacity, value,
        &table->length);
}
// HASH TABLE IMPLEMENTATION

#endif // RESMAN_IMPLEMENTATION