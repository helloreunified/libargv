# build
just include it like a normal file
# minimum requirement
`cplusplus11.hpp`: C++11 or newer  
`c11.h`: C11 or newer
# notes
every flag actually requires a value and the library may read incorrectly on incorrect user inputs
a flag is implicitly pushed without setting a value when it precedes another flag  
for a positional argument, it tries to push onto the previous token first if there's no `-` or `--` beforehand and the previous token is a flag. if it can't push to any flag, then it sits on its own (still accept comma-seperated tokenizing)  
default arg_t value: `{ chain_t tokens = {}; bool is_flag = true; bool boolean = true }`  
(case-insensitive) values considered as booleans: `0`, `(integer != 0)`, `no`, `yes`, `n`, `y`

# examples
in commandline: 
`--debug -p 192.168.0.2:1234` means `{ {"debug"}, true, true }, { {"p", "192.168.0.2:1234"}, true, true } `  
`-dd /mount/disk.qcow2,cd-rom` means `{ {"dd", "/mount/disk.qcow2", "cd-rom"}, true, true }`
`--debug 0 ./program` means `{ {"debug"}, true, false }, { {"program"}, false, true }`

```cpp
#include <iostream>
#include "cplusplus11.hpp"

sign main(int, const char** argv) {
    libargv::alias_t aliases = {
        {"port", 'p'},
        {"mem", 'm'}
    };

    libargv parser;
    // argv, should surpress, should cluster, alias
    parser.init(argv, false, true, aliases);

	// see what's inside
    for (const libargv::arg_t& arg : parser.argvect) {
        if (arg.is_flag) {
            std::cout << "[FLAG] ";
            for (const std::string& token : arg.text) {
                std::cout << "'" << token << "' ";
            }
            std::cout << "| Boolean state: " << (arg.boolean ? "true" : "false") << "\n";
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

```c
#include <stdio.h>
#include "c11.h"

signed main(int argc, const char** argv) {
    // optionally define aliases e.g. --mem to -m
    alias_t aliases[] = {
        {"port", 'p'},
        {"mem", 'm'}
    };
    // you need to keep count
    size_t alias_count = sizeof(aliases) / sizeof(aliases[0]);

    // init the parser
    libargv_parser parser;
    // what, argc, argv, should surpress, should cluster, alias, alias count
    libargv_init(&parser, argc, argv, false, true, aliases, alias_count);

    // iterate to see what's inside
    for (size_t i = 0; i < parser.argvect.size; i++) {
        arg_t* arg = (arg_t*)parser.argvect.data[i];

        if (arg->is_flag) {
            printf("[FLAG] ");
            // print tokens
            for (size_t j = 0; j < arg->text.size; j++) {
                printf("'%s' ", (char*)arg->text.data[j]);
            }
            printf("| Boolean state: %s\n", arg->boolean ? "true" : "false");
        } else {
            printf("[POSITIONAL / VALUE] ");
            for (size_t j = 0; j < arg->text.size; j++) {
                printf("'%s' ", (char*)arg->text.data[j]);
            }
            printf("\n");
        }
    }

    // prevent memory leaks
    libargv_free(&parser);
    return 0;
}
```
