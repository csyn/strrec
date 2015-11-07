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

#include "strrec.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
This does the recognition, call for each char in the buffer in order with the
same state structure return 1 << (matched pattern)
 */

static void strrec_backtrack(const char cin,
                             struct strrec_pattern * const pattern)
{
    // lower bound indicates the first char that can not be a match
    size_t part_match = 0, lower_bound = 1;
    while (lower_bound + part_match < pattern->counter)
    {
        if (pattern->pattern[lower_bound + part_match] ==
                pattern->pattern[part_match])
        {
            ++part_match;
        }
        else
        {
            part_match = 0;
            ++lower_bound;
        }
    }
    pattern->counter -= lower_bound;
    if (cin == pattern->pattern[pattern->counter])
    {
        ++pattern->counter;
    }
    else if (pattern->counter)
    {
        strrec_backtrack(cin, pattern);
    }
}

int strrec(const char cin, struct strrec_pattern * const pattern)
{
    int ret = 0;
    if (cin == pattern->pattern[pattern->counter])
    { // if there is a character match
        ++pattern->counter;
        if (pattern->counter == pattern->width)
        {
            ret = 1; // entire pattern matched
            --pattern->counter;
            if (pattern->counter)
            {
                strrec_backtrack(cin, pattern);
            }
        }
    }
    else if (pattern->counter) // if there is not a character match
    {
        strrec_backtrack(cin, pattern);
    }
    return ret;
}

int strrec_push_pattern(struct strrec_parser * const parser,
                        const char* const _pattern,
                        const size_t pattern_width,
                        int(*const callback_idle) (void*),
                        int(*const callback_update) (char, void*),
                        void* const callback_idle_arg,
                        void* const callback_update_arg)
{
    struct strrec_callback_state* c = &parser->state;
    struct strrec_match_state* m = &c->match_state;

    // handle overflow with error
    if (m->width > sizeof (c->matches) * CHAR_BIT)
    {
        return -1;
    }

    char *pattern;
    if (!(pattern = strrec_allocate(pattern_width)))
    {
        return -1;
    }
    memcpy(pattern, _pattern, pattern_width);

    m->width++;

    struct strrec_pattern *tmp_patterns;
    if (!(tmp_patterns = strrec_reallocate(m->patterns, m->width
                                           * sizeof (*(m->patterns)))))
    {
        strrec_deallocate(pattern);
        --m->width;
        return -1;
    }
    m->patterns = tmp_patterns;
    m->patterns[m->width - 1].pattern = pattern;
    m->patterns[m->width - 1].counter = 0;
    m->patterns[m->width - 1].width = pattern_width;

    struct strrec_callback *tmp_callbacks;
    if (!(tmp_callbacks = strrec_reallocate(c->callbacks, m->width
                                            * sizeof (*(c->callbacks)))))
    {
        --m->width;
        if (!(tmp_patterns = strrec_reallocate(m->patterns, m->width
                                               * sizeof (*(m->patterns)))))
        {
            return -2; // not failed safe
        }
        m->patterns = tmp_patterns;
        return -1;
    }
    c->callbacks = tmp_callbacks;
    c->callbacks[m->width - 1].callback_idle = callback_idle;
    c->callbacks[m->width - 1].callback_update = callback_update;
    c->callbacks[m->width - 1].callback_idle_arg = callback_idle_arg;
    c->callbacks[m->width - 1].callback_update_arg = callback_update_arg;

    return 0;
}

int strrec_push_string(struct strrec_parser * const parser,
                       const char* const pattern,
                       int(*const callback_idle) (void*),
                       int(*const callback_update) (char, void*),
                       void* const callback_idle_arg,
                       void* const callback_update_arg)
{
    return strrec_push_pattern(parser, pattern,
                               (unsigned int) strlen(pattern),
                               callback_idle, callback_update,
                               callback_idle_arg, callback_update_arg);
}

// selectively removes a pattern and its callback from the list

int strrec_remove_pattern(struct strrec_parser * const parser,
                          const char* const pattern,
                          const size_t pattern_width)
{
    struct strrec_callback_state *c = &parser->state;
    struct strrec_match_state *m = &c->match_state;

    unsigned int i = 0;
    unsigned char remove = 0;
    while (i < m->width)
    {
        if (!remove)
        {
            unsigned int match_count = 0;
            if (pattern_width == m->patterns[i].width)
            { // if the pattern is the same width as the one in the list
                while (match_count < pattern_width && pattern[match_count]
                        == m->patterns[i].pattern[match_count])
                {
                    match_count++;
                }

                if (match_count == pattern_width) // compare list items
                {
                    strrec_deallocate(m->patterns[i].pattern);
                    remove = 1;
                }
            }
        }
        else
        {
            c->callbacks[i - 1] = c->callbacks[i];
            m->patterns[i - 1] = m->patterns[i];
        }
        i++;
    }
    if (remove)
    {
        m->width--;

        struct strrec_pattern *tmp_patterns;
        if (!(tmp_patterns = strrec_reallocate(m->patterns, m->width
                                               * sizeof (*(m->patterns)))))
        {
            return -1;
        }
        m->patterns = tmp_patterns;
        struct strrec_callback *tmp_callbacks;
        if (!(tmp_callbacks = strrec_reallocate(c->callbacks, m->width
                                                * sizeof (*(c->callbacks)))))
        {
            return -1;
        }
        c->callbacks = tmp_callbacks;
    }
    return 0;
}

int strrec_remove_string(struct strrec_parser * const parser,
                         const char* pattern)
{
    return strrec_remove_pattern(parser, pattern, strlen(pattern));
}

int strrec_idle(struct strrec_parser * const parser)
{
    int ret = 0;
    if (parser->state.current_input_valid > 0)
    {
        if ((parser->state.matches & (1 << parser->state.current_pattern)) != 0)
        {
            if (parser->state.callbacks[parser->state.current_pattern]
                    .callback_update != 0)
            {
                if ((ret = (*(parser->state
                        .callbacks[parser->state.current_pattern]
                        .callback_update))
                        (parser->state.current_input,
                        parser->state.callbacks[parser->state.current_pattern]
                        .callback_update_arg)) != 0) // execute the callback
                { // clear the match
                    parser->state.matches &= ~(1 <<
                            parser->state.current_pattern);
                }
            }
        }
        parser->state.matches |=
                (strrec(parser->state.current_input, &parser->state.match_state
                        .patterns[parser->state.current_pattern])
                << parser->state.current_pattern); // modify matches
    }
    if (ret == 0)
    {
        if (parser->state.matches & (1 << parser->state.current_pattern))
        {
            if (parser->state.callbacks[parser->state.current_pattern]
                    .callback_idle)
            {
                if ((ret = (*(parser->state
                        .callbacks[parser->state.current_pattern]
                        .callback_idle))
                        (parser->state.callbacks[parser->state.current_pattern]
                        .callback_idle_arg)) != 0) // execute the callback
                { // clear the match
                    parser->state.matches &= ~(1 <<
                            parser->state.current_pattern);
                }
            }
        }
        ++parser->state.current_pattern;
        if (parser->state.current_pattern == parser->state.match_state.width)
        {
            parser->state.current_pattern = 0;
            parser->state.current_input_valid =
                    parser->get_char(&parser->state.current_input);
            if (parser->state.current_input_valid < 0)
            {
                return parser->state.current_input_valid;
            }
        }
    }
    return ret;
}

void strrec_destroy(struct strrec_parser * const parser)
{
    struct strrec_callback_state *c = &parser->state;
    struct strrec_match_state *m = &c->match_state;
    // deallocate patterns
    size_t i = 0;
    while (i < m->width)
    {
        strrec_deallocate(m->patterns[i].pattern);
        i++;
    }

    if (m->patterns)
    {
        strrec_deallocate(m->patterns);
    }
    if (c->callbacks)
    {
        strrec_deallocate(c->callbacks);
    }
}

struct strrec_parser strrec_init(int (*get_char)(char *c))
{
    struct strrec_parser parser = {get_char,
        {
            {0}
        }};
    return parser;
}
