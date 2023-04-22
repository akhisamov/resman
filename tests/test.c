#include "test.h"

#define RESMAN_IMPLEMENTATION
#include "resman.h"

#define TEST_STRING "TEST"

void create_and_destroy_test()
{
    resman_storage* storage = resman_create("base_dir");
    ASSERT(storage != NULL);

    EXPECT(strcmp(storage->base_dir, "base_dir") == 0);

    resman_destroy(&storage);
    ASSERT(storage == NULL);
}

resman_resource* resource_factory(const char* path)
{
    char* test_resource = calloc(strlen(path) + 1, sizeof(char));
    if (test_resource == NULL)
    {
        return NULL;
    }
    strcpy(test_resource, path);
    return test_resource;
}

void register_factory_test()
{
    resman_storage* storage = resman_create("base_dir");
    ASSERT(storage != NULL);

    RESMAN_REGISTER_FACTORY(storage, string, resource_factory);
    ASSERT(storage->caches != NULL);
    ASSERT(storage->caches->length > 0);

    resman_destroy(&storage);
    ASSERT(storage == NULL);
}

void load_and_unload_resource_test()
{
    resman_storage* storage = resman_create("base_dir");
    ASSERT(storage != NULL);

    RESMAN_REGISTER_FACTORY(storage, string, resource_factory);
    ASSERT(storage->caches != NULL);
    ASSERT(storage->caches->length > 0);

    const char* path = "test-path";
    resman_resource* resource = RESMAN_LOAD(storage, string, path);
    ASSERT(resource != NULL);

    EXPECT(strcmp(resource, path) == 0);

    RESMAN_UNLOAD(storage, string, &resource);
    ASSERT(resource == NULL);

    resman_destroy(&storage);
    ASSERT(storage == NULL);
}

int main()
{
    RUN_TEST(create_and_destroy_test);
    RUN_TEST(register_factory_test);
    RUN_TEST(load_and_unload_resource_test);
    return 0;
}