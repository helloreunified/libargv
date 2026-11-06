/*
	repository: https://github.com/helloreunified/libargv
	file: c11.h

	This software is licensed under the MIT License.
	Permission is hereby granted, free of charge,
	to any person obtaining a copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation the rights to use, copy,
	modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
	INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef libargv_libargv
#define libargv_libargv

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    void** data;
    size_t size;
    size_t capacity;
} vec_t;

static void libargv_vec_init(vec_t* v, size_t initial_cap) {
    v->size = 0;
    v->capacity = initial_cap ? initial_cap : 4;
    v->data = malloc(v->capacity * sizeof(void*));
}

static void libargv_vec_push(vec_t* v, void* item) {
    if (v->size >= v->capacity) {
        v->capacity *= 2;
        v->data = realloc(v->data, v->capacity * sizeof(void*));
    }
    v->data[v->size++] = item;
}

static void libargv_vec_free(vec_t* v) {
    free(v->data);
    v->data = NULL;
    v->size = v->capacity = 0;
}

typedef struct {
    vec_t text;
    bool is_flag;
} arg_t;

typedef struct {
    const char* key;
    char val;
} alias_t;

typedef struct {
    vec_t argvect;
} libargv_parser;

static void libargv_found_hyphen(bool* surpress_invalid, const char* text) {
    if (*surpress_invalid == false) return;
    fprintf(stderr, "[libargv] WARNING; found '-' with is_clustered == true. This is probably an user error/ wrong CI command; or encountering non-standard flag styles like -memory or -disk-drive. Clustered flags may look like -xvf, meaning -x -v -f. You should set surpress_invalid = true for production builds. Faulty flag: '%s'\n", text);
}

static vec_t libargv_argtok(const char* user_str) {
    vec_t ret;
    libargv_vec_init(&ret, 2);
    
    size_t len = strlen(user_str);
    size_t prev = 0;
    for (size_t i = 0; i <= len; i++) {
        if (user_str[i] == ',' || user_str[i] == '\0') {
            size_t sub_len = i - prev;
            char* sub = malloc(sub_len + 1);
            memcpy(sub, user_str + prev, sub_len);
            sub[sub_len] = '\0';
            libargv_vec_push(&ret, sub);
            prev = i + 1;
        }
    }
    return ret;
}

static char libargv_alias_find(const alias_t* user, size_t user_count, const char* name) {
    if (!user) return '\0';
    for (size_t i = 0; i < user_count; i++) {
        if (strcmp(user[i].key, name) == 0) return user[i].val;
    }
    return '\0';
}

static void libargv_init(libargv_parser* parser, int argc, const char** argv, 
                         bool surpress_invalid, bool is_clustered, 
                         const alias_t* user, size_t user_count) {
    libargv_vec_init(&parser->argvect, 4);

    arg_t* hold = NULL;

    for (int i = 1; i < argc; i++) {
        const char* curr = argv[i];
        size_t curr_len = strlen(curr);
        if (curr_len < 1) continue;

        if (curr[0] != '-') {
            if (!hold) {
                arg_t* arg = malloc(sizeof(arg_t));
                arg->text = libargv_argtok(curr);
                arg->is_flag = false;
                libargv_vec_push(&parser->argvect, arg);
            } else {
                vec_t tokens = libargv_argtok(curr);
                for (size_t t = 0; t < tokens.size; t++) {
                    libargv_vec_push(&hold->text, tokens.data[t]);
                }
                libargv_vec_free(&tokens);
                libargv_vec_push(&parser->argvect, hold);
                hold = NULL;
            }
            continue;
        }

        if (hold) {
            libargv_vec_push(&parser->argvect, hold);
            hold = NULL;
        }

        if (strcmp(curr, "-") == 0 || strcmp(curr, "--") == 0) {
            arg_t* arg = malloc(sizeof(arg_t));
            arg->text = libargv_argtok(curr);
            arg->is_flag = false;
            libargv_vec_push(&parser->argvect, arg);
            continue;
        }

        if (is_clustered) {
            if (curr[1] != '-') {
                for (size_t x = 1; x < curr_len; x++) {
                    if (curr[x] == '-') {
                        bool temp_sup = surpress_invalid;
                        libargv_found_hyphen(&temp_sup, curr);
                    }
                    char flag_str[3] = {'-', curr[x], '\0'};
                    arg_t* arg = malloc(sizeof(arg_t));
                    arg->text = libargv_argtok(flag_str);
                    arg->is_flag = true;
                    libargv_vec_push(&parser->argvect, arg);
                }
                continue;
            }

            if (!hold) {
                hold = malloc(sizeof(arg_t));
                libargv_vec_init(&hold->text, 2);
                hold->is_flag = true;
            }
            const char* name = curr + 2;
            char alias_val = libargv_alias_find(user, user_count, name);
            if (alias_val != '\0') {
                char* s = malloc(2); s[0] = alias_val; s[1] = '\0';
                libargv_vec_push(&hold->text, s);
            } else {
                libargv_vec_push(&hold->text, strdup(name));
            }
        } else {
            if (!hold) {
                hold = malloc(sizeof(arg_t));
                libargv_vec_init(&hold->text, 2);
                hold->is_flag = true;
            }
            const char* name = curr + 1;
            char alias_val = libargv_alias_find(user, user_count, name);
            if (alias_val != '\0') {
                char* s = malloc(2); s[0] = alias_val; s[1] = '\0';
                libargv_vec_push(&hold->text, s);
            } else {
                libargv_vec_push(&hold->text, strdup(name));
            }
        }
    }

    if (hold) {
        libargv_vec_push(&parser->argvect, hold);
    }
}

static void libargv_free(libargv_parser* parser) {
    for (size_t i = 0; i < parser->argvect.size; i++) {
        arg_t* arg = (arg_t*)parser->argvect.data[i];
        for (size_t j = 0; j < arg->text.size; j++) {
            free(arg->text.data[j]);
        }
        libargv_vec_free(&arg->text);
        free(arg);
    }
    libargv_vec_free(&parser->argvect);
}

#endif
