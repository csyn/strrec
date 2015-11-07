/*
The MIT License (MIT)

Copyright (c) 2012 Julian Ingram

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 */

#ifndef STRREC_H
#define STRREC_H

#include <stdlib.h>
#include <stdio.h>

/********** USER CONFIG **********/

static inline void* strrec_allocate(size_t size)
{
	return malloc(size);
}

static inline void* strrec_reallocate(void* pointer, size_t size)
{
	return realloc(pointer, size);
}

static inline void strrec_deallocate(void* pointer)
{
	free(pointer);
}

/*********************************/

struct strrec_pattern
{
    char* pattern;
    size_t width;
    size_t counter;
};

struct strrec_match_state
{
    size_t width;
    struct strrec_pattern* patterns;
};

struct strrec_callback
{
    int (*callback_idle)(void*);
    int (*callback_update)(char, void*);
    void* callback_idle_arg;
    void* callback_update_arg;
};

struct strrec_callback_state
{
    struct strrec_match_state match_state;
    struct strrec_callback* callbacks;
    unsigned long long int matches;
	size_t current_pattern;
	char current_input;
	char current_input_valid;
};

struct strrec_parser
{
    int (*get_char)(char*);
    struct strrec_callback_state state;
};

int strrec(char const cin, struct strrec_pattern* pattern);
int strrec_push_pattern(struct strrec_parser* const parser,
                        const char* _pattern,
                        size_t const pattern_width,
                        int(*const callback_idle) (void*),
                        int(*const callback_update) (char, void*),
                        void* const callback_idle_arg,
                        void* const callback_update_arg);
int strrec_push_string(struct strrec_parser* const parser,
                       const char* pattern,
                       int(*const callback_idle) (void*),
                       int(*const callback_update) (char, void*),
                       void* const callback_idle_arg,
                       void* const callback_update_arg);
int strrec_remove_pattern(struct strrec_parser* const parser, const char*,
                          const size_t pattern_width);
int strrec_remove_string(struct strrec_parser* const parser,
							const char* pattern);

int strrec_idle(struct strrec_parser* const parser);
void strrec_destroy(struct strrec_parser* const parser);
struct strrec_parser strrec_init(int (*get_char)(char* c));

#endif
