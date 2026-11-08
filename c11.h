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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} libargv_chain_t;

typedef struct {
    libargv_chain_t text;
    bool is_flag;
    bool boolean;
} libargv_arg_t;

typedef struct {
    const char *name;
    char alias;
} libargv_alias_t;

typedef struct {
    libargv_arg_t *argvect;
    size_t count;
    size_t capacity;
} libargv_t;

typedef uint64_t libargv_hash_t;

typedef struct {
    char *name;
    char alias;
    libargv_hash_t hash[4];
} libargv_alias_entry_t;

typedef struct {
    libargv_alias_entry_t *items;
    size_t count;
    size_t capacity;
} libargv_alias_table_t;

/* Hashing */

static inline void libargv_hash_all(
    libargv_hash_t hash[4],
    const char *str
) {
    static const libargv_hash_t base[4] = {
        257, 263, 269, 271
    };

    static const libargv_hash_t mod[4] = {
        1000000007ULL,
        1000000009ULL,
        998244353ULL,
        1000000033ULL
    };

    for (size_t j = 0; j < 4; j++)
        hash[j] = 0;

    for (size_t i = 0; str[i] != '\0'; i++) {
        unsigned char c = (unsigned char)str[i];

        for (size_t j = 0; j < 4; j++) {
            hash[j] = (hash[j] * base[j] + c) % mod[j];
        }
    }
}

static inline char *libargv_strndup(
    const char *str,
    size_t len
) {
    char *ret = malloc(len + 1);
    if (!ret) return NULL;

    memcpy(ret, str, len);
    ret[len] = '\0';

    return ret;
}

static inline bool libargv_reserve(
    void **ptr,
    size_t *capacity,
    size_t required,
    size_t element_size
) {
    if (*capacity >= required)
        return true;

    if (element_size == 0)
        return false;

    size_t next = *capacity ? *capacity : 4;

    while (next < required) {
        if (next > SIZE_MAX / 2) {
            next = required;
            break;
        }

        next *= 2;
    }

    if (next > SIZE_MAX / element_size)
        return false;

    void *tmp = realloc(*ptr, next * element_size);
    if (!tmp) return false;

    *ptr = tmp;
    *capacity = next;

    return true;
}

static inline bool libargv_is_prime(size_t n) {
    if (n < 2)
        return false;

    for (size_t i = 2; i <= n / i; i++) {
        if (n % i == 0)
            return false;
    }

    return true;
}

static inline size_t libargv_next_prime(size_t n) {
    while (!libargv_is_prime(n)) {
        if (n == SIZE_MAX)
            return 0;

        n++;
    }

    return n;
}

static inline void libargv_alias_table_free(
    libargv_alias_table_t *table
) {
    if (!table)
        return;

    for (size_t i = 0; i < table->capacity; i++)
        free(table->items[i].name);

    free(table->items);

    *table = (libargv_alias_table_t){0};
}

static inline libargv_alias_entry_t *libargv_alias_table_find(
    const libargv_alias_table_t *table,
    const char *name
) {
    if (!table || !name || table->capacity == 0)
        return NULL;

    libargv_hash_t hash[4];
    libargv_hash_all(hash, name);

    size_t index = hash[0] % table->capacity;
    size_t step = 1 + hash[2] % (table->capacity - 1);

    for (size_t i = 0; i < table->capacity; i++) {
        libargv_alias_entry_t *entry = &table->items[index];
        
        if (!entry->name)
            return NULL;
            
        if (entry->hash[0] == hash[0] &&
            entry->hash[1] == hash[1] &&
            entry->hash[2] == hash[2] &&
            entry->hash[3] == hash[3] &&
            strcmp(entry->name, name) == 0) {
            return entry;
        }

        index = (index + step) % table->capacity;
    }

    return NULL;
}

static inline bool libargv_alias_table_insert(
    libargv_alias_table_t *table,
    const libargv_alias_t *alias
) {
    if (!table || !alias || !alias->name || table->capacity == 0)
        return false;

    libargv_hash_t hash[4];
    libargv_hash_all(hash, alias->name);

    size_t index = hash[0] % table->capacity;
    size_t step = 1 + hash[2] % (table->capacity - 1);

    for (size_t i = 0; i < table->capacity; i++) {
        libargv_alias_entry_t *entry = &table->items[index];

        if (!entry->name) {
            char *copy = libargv_strndup(
                alias->name,
                strlen(alias->name)
            );

            if (!copy)
                return false;

            entry->name = copy;
            entry->alias = alias->alias;

            memcpy(entry->hash, hash, sizeof(hash));

            table->count++;
            return true;
        }

        if (entry->hash[0] == hash[0] &&
            entry->hash[1] == hash[1] &&
            entry->hash[2] == hash[2] &&
            entry->hash[3] == hash[3] &&
            strcmp(entry->name, alias->name) == 0) {

            entry->alias = alias->alias;
            return true;
        }

        index = (index + step) % table->capacity;
    }

    return false;
}

static inline bool libargv_alias_table_init(
    libargv_alias_table_t *table,
    const libargv_alias_t *aliases,
    size_t count
) {
    if (!table || (count > 0 && !aliases))
        return false;

    *table = (libargv_alias_table_t){0};

    if (count == 0)
        return true;

    if (count > (SIZE_MAX - 2) / 2)
        return false;

    size_t capacity = libargv_next_prime(count * 2 + 1);

    if (capacity == 0 ||
        capacity > SIZE_MAX / sizeof(libargv_alias_entry_t)) {
        return false;
    }

    table->items = calloc(
        capacity,
        sizeof(libargv_alias_entry_t)
    );

    if (!table->items)
        return false;

    table->capacity = capacity;

    for (size_t i = 0; i < count; i++) {
        if (!aliases[i].name ||
            !libargv_alias_table_insert(table, &aliases[i])) {
            libargv_alias_table_free(table);
            return false;
        }
    }

    return true;
}

static inline bool libargv_chain_push(
    libargv_chain_t *chain,
    char *text
) {
    if (!libargv_reserve(
        (void **)&chain->items,
        &chain->capacity,
        chain->count + 1,
        sizeof(char *)
    )) {
        return false;
    }

    chain->items[chain->count++] = text;
    return true;
}

static inline bool libargv_chain_push_copy(
    libargv_chain_t *chain,
    const char *text
) {
    char *copy = libargv_strndup(text, strlen(text));

    if (!copy)
        return false;

    if (!libargv_chain_push(chain, copy)) {
        free(copy);
        return false;
    }

    return true;
}

static inline void libargv_chain_clear(
    libargv_chain_t *chain
) {
    for (size_t i = 0; i < chain->count; i++)
        free(chain->items[i]);

    chain->count = 0;
}

static inline void libargv_chain_free(
    libargv_chain_t *chain
) {
    libargv_chain_clear(chain);
    free(chain->items);

    chain->items = NULL;
    chain->capacity = 0;
}

static inline bool libargv_arg_empty(
    const libargv_arg_t *arg
) {
    return arg->text.count == 0;
}

static inline void libargv_arg_clear(
    libargv_arg_t *arg
) {
    libargv_chain_clear(&arg->text);
    arg->is_flag = true;
    arg->boolean = true;
}

static inline void libargv_arg_free(
    libargv_arg_t *arg
) {
    libargv_chain_free(&arg->text);

    arg->is_flag = true;
    arg->boolean = true;
}

static inline bool libargv_push(
    libargv_t *args,
    libargv_arg_t *arg
) {
    if (!libargv_reserve(
        (void **)&args->argvect,
        &args->capacity,
        args->count + 1,
        sizeof(libargv_arg_t)
    )) {
        return false;
    }

    args->argvect[args->count++] = *arg;

    arg->text.items = NULL;
    arg->text.count = 0;
    arg->text.capacity = 0;

    return true;
}

static inline void libargv_free(
    libargv_t *args
) {
    if (!args)
        return;

    for (size_t i = 0; i < args->count; i++)
        libargv_arg_free(&args->argvect[i]);

    free(args->argvect);

    args->argvect = NULL;
    args->count = 0;
    args->capacity = 0;
}

static inline void libargv_found_hyphen(
    bool surpress_invalid,
    const char *text
) {
    if (surpress_invalid)
        return;

    fprintf(
        stderr,
        "[libargv] WARNING: found '-' with is_clustered == true. "
        "Faulty flag: '%s'\n",
        text
    );
}

static inline bool libargv_booleval(
    libargv_arg_t *hold,
    const char *input
) {
    size_t len = strlen(input);

    char *thing = libargv_strndup(input, len);
    if (!thing)
        return false;

    for (size_t i = 0; i < len; i++) {
        if (thing[i] >= 'A' && thing[i] <= 'Z')
            thing[i] += 32;
    }

    bool matched = true;

    if (strcmp(thing, "no") == 0 ||
        strcmp(thing, "n") == 0 ||
        strcmp(thing, "false") == 0) {
        hold->boolean = false;
    }
    else if (strcmp(thing, "yes") == 0 ||
             strcmp(thing, "y") == 0 ||
             strcmp(thing, "true") == 0) {
        hold->boolean = true;
    }
    else {
        matched = false;
    }

    free(thing);
    return matched;
}

static inline bool libargv_argtok(
    libargv_chain_t *ret,
    const char *user
) {
    size_t prev = 0;
    size_t len = strlen(user);

    for (size_t i = 0; i < len; i++) {
        if (user[i] == ',') {
            char *token = libargv_strndup(
                user + prev,
                i - prev
            );

            if (!token)
                return false;

            if (!libargv_chain_push(ret, token)) {
                free(token);
                return false;
            }

            prev = i + 1;
        }
    }

    if (prev < len) {
        char *token = libargv_strndup(
            user + prev,
            len - prev
        );

        if (!token)
            return false;

        if (!libargv_chain_push(ret, token)) {
            free(token);
            return false;
        }
    }

    return true;
}

static inline bool libargv_init(
    libargv_t *args,
    int argc,
    const char *const argv[],
    bool surpress_invalid,
    bool is_clustered,
    const libargv_alias_t *user,
    size_t user_count
) {
    if (!args)
        return false;

    libargv_free(args);

    if (argc < 0 || (argc > 0 && !argv))
        return false;

    libargv_alias_table_t aliases = {0};

    if (!libargv_alias_table_init(
        &aliases,
        user,
        user_count
    )) {
        return false;
    }

    bool pure_positional = false;

    libargv_arg_t hold = {0};
    hold.is_flag = true;
    hold.boolean = true;

    for (int i = 1; i < argc; i++) {
        const char *current = argv[i];

        if (!current)
            break;

        if (current[0] == '\0')
            continue;

        if (pure_positional || current[0] != '-') {
            if (libargv_arg_empty(&hold)) {
                libargv_arg_t positional = {0};
                positional.is_flag = false;
                positional.boolean = true;

                if (!libargv_argtok(
                    &positional.text,
                    current
                )) {
                    libargv_arg_free(&positional);
                    goto failure;
                }

                if (!libargv_push(args, &positional)) {
                    libargv_arg_free(&positional);
                    goto failure;
                }
            }
            else {
                if (libargv_booleval(&hold, current))
                    continue;

                if (!libargv_argtok(&hold.text, current))
                    goto failure;

                if (!libargv_push(args, &hold))
                    goto failure;

                libargv_arg_clear(&hold);
            }

            continue;
        }

        if (!libargv_arg_empty(&hold)) {
            if (!libargv_push(args, &hold))
                goto failure;

            libargv_arg_clear(&hold);
        }

        if (strcmp(current, "-") == 0 ||
            strcmp(current, "--") == 0) {
            libargv_arg_t positional = {0};
            positional.is_flag = false;
            positional.boolean = true;

            if (!libargv_argtok(
                &positional.text,
                current
            )) {
                libargv_arg_free(&positional);
                goto failure;
            }

            if (!libargv_push(args, &positional)) {
                libargv_arg_free(&positional);
                goto failure;
            }

            pure_positional = true;
            continue;
        }

        if (is_clustered) {
            if (current[1] != '-') {

                for (size_t x = 1; current[x] != '\0'; x++) {
                    if (current[x] == '-') {
                        libargv_found_hyphen(
                            surpress_invalid,
                            current
                        );
                    }

                    char flag_str[2] = {
                        current[x],
                        '\0'
                    };

                    if (!libargv_chain_push_copy(
                        &hold.text,
                        flag_str
                    )) {
                        goto failure;
                    }
                }

                continue;
            }
            
            const char *name = current + 2;

            libargv_alias_entry_t *alias =
                libargv_alias_table_find(&aliases, name);

            if (alias) {
                char value[2] = {
                    alias->alias,
                    '\0'
                };

                if (!libargv_chain_push_copy(
                    &hold.text,
                    value
                )) {
                    goto failure;
                }
            }
            else {
                if (!libargv_chain_push_copy(
                    &hold.text,
                    name
                )) {
                    goto failure;
                }
            }
        }
        else {
            const char *name = current + 1;

            libargv_alias_entry_t *alias =
                libargv_alias_table_find(&aliases, name);

            if (alias) {
                char value[2] = {
                    alias->alias,
                    '\0'
                };

                if (!libargv_chain_push_copy(
                    &hold.text,
                    value
                )) {
                    goto failure;
                }
            }
            else {
                if (!libargv_chain_push_copy(
                    &hold.text,
                    name
                )) {
                    goto failure;
                }
            }
        }
    }

    if (!libargv_arg_empty(&hold)) {
        if (!libargv_push(args, &hold))
            goto failure;
    }

    libargv_arg_free(&hold);
    libargv_alias_table_free(&aliases);

    return true;

failure:
    libargv_arg_free(&hold);
    libargv_alias_table_free(&aliases);
    libargv_free(args);

    return false;
}

#endif
