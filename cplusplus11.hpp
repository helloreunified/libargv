/*
	repository: https://github.com/helloreunified/libargv
	file: cplusplus11.hpp

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

#ifndef libargv_libargv_libargv
#define libargv_libargv_libargv

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <cstdio>

class libargv {
    public:
        typedef std::unordered_map<std::string, char> alias_t;
        typedef std::vector<std::string> chain_t;
        
        struct arg_t {
            chain_t text;
            bool is_flag = true;

            arg_t() = default;
            arg_t(chain_t _t, bool _i)
                : text(std::move(_t)), is_flag(_i) {}
            bool is_empty() { return text.empty(); }
            void clear() { text.clear(); is_flag = true; }
        };

        std::vector<arg_t> argvect;

        void init(const char**& argv, bool surpress_invalid = false, bool is_clustered = true, alias_t user = {}) {
            argvect.clear();
            argvstr(argv);
            arg_t hold;
            for (size_t i = 1; i < strvect.size(); i++) {
                // low quality argv injection prevention
                if (strvect[i].size() < 1) continue;

                if (strvect[i].size() >= 1 && strvect[i][0] != '-') {
                    if (hold.is_empty()) // is a positional
                        argvect.emplace_back(argtok(strvect[i]), false);
                    else { // be after a flag
                        chain_t tokens = argtok(strvect[i]);
                        hold.text.insert(hold.text.end(), tokens.begin(), tokens.end());

                        argvect.push_back(std::move(hold));
                        hold.clear();
                    }
                    continue;
                }

                // now they're strictly flags, push it if no positional
                if (!hold.is_empty()) {
                    argvect.push_back(std::move(hold));
                    hold.clear();
                }
                
                // purely positionals
                if (strvect[i] == "-" || strvect[i] == "--") {
                    argvect.emplace_back(argtok(strvect[i]), false);
                    continue;
                }

                if (is_clustered) {
                    if (strvect[i][1] != '-') {
                        for (size_t x = 1; x < strvect[i].size(); x++) {
                            if (strvect[i][x] == '-')
                                found_hyphen(surpress_invalid, strvect[i]);
                            std::string flag_str = "-" + std::string (1, strvect[i][x]);
                            hold.text.push_back(flag_str);
                        }
                        continue;
                    }
                    
                    std::string name = strvect[i].substr(2);
                    auto iterator_ret = user.find(name);
                    if (iterator_ret != user.end())
                        hold.text.push_back(std::string (1, iterator_ret->second));
                    else
                        hold.text.push_back(name);
                }
                else {
                    std::string name = strvect[i].substr(1);
                    auto iterator_ret = user.find(name);
                    if (iterator_ret != user.end())
                        hold.text.push_back(std::string (1, iterator_ret->second));
                    else
                        hold.text.push_back(name);
                }
            }    

            if (!hold.is_empty())
                argvect.push_back(std::move(hold));
        }

    private:
        std::vector<std::string> strvect;

        void found_hyphen(bool& surpress_invalid, std::string& text) {
            if (surpress_invalid) return;
            std::fprintf(stderr, "[libargv] WARNING: found '-' with is_clustered == true. Faulty flag: '%s'\n", text.c_str());
        }

        void argvstr(const char**& argv) {
            strvect.clear();
            for (int i = 0; argv[i] != 0; i++)
                strvect.push_back(argv[i]);
        }
        
        chain_t argtok(std::string& user) {
            chain_t ret;
            size_t prev = 0;
            for (size_t i = 0; i < user.size(); i++)
                if (user[i] == ',') {
                    ret.push_back(user.substr(prev, i - prev));
                    prev = i + 1;
                }
            if (prev < user.size())
                ret.push_back(user.substr(prev));
            return ret;
        }
};

#endif
