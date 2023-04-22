#ifndef TEST_H
#define TEST_H

#include <stdlib.h>
#include <stdio.h>

#define EXPECT(condition) printf("[\x1b[34m%s\x1b[0m] %s is %s\x1b[0m\n", __func__, #condition, condition ? "\x1b[32mTRUE" : "\x1b[31mFALSE");
#define ASSERT(condition) printf("[\x1b[34m%s\x1b[0m] %s is %s\x1b[0m\n", __func__, #condition, condition ? "\x1b[32mTRUE" : "\x1b[31mFALSE"); if (!(condition)) abort();
#define RUN_TEST(test) printf("\n[\x1b[34m%s\x1b[0m] \x1b[33mBEGIN\x1b[0m\n", #test); test(); printf("[\x1b[34m%s\x1b[0m] \x1b[33mEND\x1b[0m\n", #test);

#endif // TEST_H