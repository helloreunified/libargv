# build
just include it like a normal file

# minimum requirement
- `cplusplus11.hpp`: C++11 or newer
- `c11.h`: C11 or newer

# notes
every flag is intended to receive a value  
the library does not enforce this and may parse incorrect user inputs unexpectedly

a flag is implicitly pushed without receiving a value when another flag precedes its value

for a positional argument, the parser first tries to append it to the previous token if that token is a flag  
if it cannot, the positional argument is pushed on its own (comma-separated tokenization is still supported)

default `arg_t` value  
```cpp
{
    chain_t text = {};
    bool is_flag = true;
    bool boolean = true;
}
```

case-insensitive values considered as booleans  
- `true`, `false`
- `yes`, `no`
- `y`, `n`

# examples
in command line:

**clustering enabled**  
```bash
--debug -p 192.168.0.2:1234
```  
means  
```text
{ {"debug"}, true, true },
{ {"p", "192.168.0.2:1234"}, true, true }
```

**clustering disabled**  
```bash
-dd /mount/disk.qcow2,cd-rom
```  
means  
```text
{ {"dd", "/mount/disk.qcow2", "cd-rom"}, true, true }
```

**Boolean value**  
```bash
./program ./input --debug false
```  
means  
```text
{ {"./input"}, false, true },
{ {"debug"}, true, false }
```

C++ example

```cpp
#include <iostream>
#include "cplusplus11.hpp"

int main(int, char** argv) {
    libargv_t::alias_t aliases = {
        {"port", 'p'},
        {"mem", 'm'}
    };

    libargv_t parser;
    // argv, should suppress, should cluster, aliases
    parser.init(argv, false, true, aliases);

    // see what's inside
    for (const libargv_t::arg_t& arg : parser.argvect) {
        if (arg.is_flag) {
            std::cout << "[FLAG] ";
            for (const std::string& token : arg.text) {
                std::cout << "'" << token << "' ";
            }
            std::cout << "| Boolean state: "
                      << (arg.boolean ? "true" : "false") << "\n";
        } else {
            std::cout << "[POSITIONAL / VALUE] ";
            for (const std::string& token : arg.text) {
                std::cout << "'" << token << "' ";
            }
            std::cout << "\n";
        }
    }

    return 0;
}
```

C example

```c
#include <stdio.h>
#include <stdbool.h>
#include "c11.h"

int main(int argc, char **argv) {
    // optionally define aliases, e.g. --mem to -m
    libargv_alias_t aliases[] = {
        {"port", 'p'},
        {"mem", 'm'},
        {"verbose", 'v'}
    };

    // you need to keep count
    size_t alias_count = sizeof(aliases) / sizeof(aliases[0]);

    // init the parser
    libargv_t parser = {0};

    // args, argc, argv, suppress invalid, cluster, aliases, alias count
    if (!libargv_init(
        &parser,
        argc,
        (const char *const *)argv,
        false,
        true,
        aliases,
        alias_count
    )) {
        fprintf(stderr, "libargv failed.\n");
        libargv_free(&parser);
        return 1;
    }

    // iterate to see what's inside
    for (size_t i = 0; i < parser.count; i++) {
        libargv_arg_t *arg = &parser.argvect[i];

        if (arg->is_flag) {
            printf("[FLAG] ");

            for (size_t j = 0; j < arg->text.count; j++) {
                printf("'%s' ", arg->text.items[j]);
            }

            printf("| Boolean state: %s\n",
                   arg->boolean ? "true" : "false");
        } else {
            printf("[POSITIONAL / VALUE] ");

            for (size_t j = 0; j < arg->text.count; j++) {
                printf("'%s' ", arg->text.items[j]);
            }

            printf("\n");
        }
    }

    // prevent memory leaks
    libargv_free(&parser);
    return 0;
}
```
