#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stddef.h>
#include <stdarg.h>

#include "mem_alloc.h"

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#define nullptr NULL

typedef unsigned short wchar;

#define class_decl(classname) typedef struct classname ## _t classname
#define class(classname) class_decl(classname); struct classname ## _t

#define TEST

#define LF (char)0x0a

#define new(classname, ...) classname ## _constructor(mem_alloc_zero(classname, 1), ##__VA_ARGS__)
#define delete(classname, ptr) do { \
	if (!ptr) \
		break; \
	classname ## _distructor(ptr); \
	mem_free(ptr); \
}while (0)

#define constructor(classname, ...) classname *classname ## _constructor(classname *self, ##__VA_ARGS__)
#define constructor_end return self
#define distructor(classname) void classname ## _distructor(classname *self)