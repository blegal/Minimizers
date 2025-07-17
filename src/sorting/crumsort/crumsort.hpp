// crumsort 1.2.1.3 - Igor van den Hoven ivdhoven@gmail.com

#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdalign.h>
#include <float.h>
#include <string.h>

typedef int CMPFUNC (const void *a, const void *b);

extern void crumsort_prim(void *array, size_t nmemb, size_t size);
extern void crumsort(void *array, size_t nmemb, size_t size, CMPFUNC *cmp);
