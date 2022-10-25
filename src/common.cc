#include "common.h"

#include <unistd.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <regex>
#include <string>
#include <unordered_map>


const std::vector<std::vector<int>> color_list = {
    {220, 20, 60}, {119, 11, 32}, {0, 0, 142}, {0, 0, 230},
    {106, 0, 228}, {0, 60, 100}, {0, 80, 100}, {0, 0, 70},
    {0, 0, 192}, {250, 170, 30}, {100, 170, 30}, {220, 220, 0},
    {175, 116, 175}, {250, 0, 30}, {165, 42, 42}, {255, 77, 255},
    {0, 226, 252}, {182, 182, 255}, {0, 82, 0}, {120, 166, 157},
    {110, 76, 0}, {174, 57, 255}, {199, 100, 0}, {72, 0, 118},
    {255, 179, 240}, {0, 125, 92}, {209, 0, 151}, {188, 208, 182},
    {0, 220, 176}, {255, 99, 164}, {92, 0, 73}, {133, 129, 255},
    {78, 180, 255}, {0, 228, 0}, {174, 255, 243}, {45, 89, 255},
    {134, 134, 103}, {145, 148, 174}, {255, 208, 186},
    {197, 226, 255}, {171, 134, 1}, {109, 63, 54}, {207, 138, 255},
    {151, 0, 95}, {9, 80, 61}, {84, 105, 51}, {74, 65, 105},
    {166, 196, 102}, {208, 195, 210}, {255, 109, 65}, {0, 143, 149},
    {179, 0, 194}, {209, 99, 106}, {5, 121, 0}, {227, 255, 205},
    {147, 186, 208}, {153, 69, 1}, {3, 95, 161}, {163, 255, 0},
    {119, 0, 170}, {0, 182, 199}, {0, 165, 120}, {183, 130, 88},
    {95, 32, 0}, {130, 114, 135}, {110, 129, 133}, {166, 74, 118},
    {219, 142, 185}, {79, 210, 114}, {178, 90, 62}, {65, 70, 15},
    {127, 167, 115}, {59, 105, 106}, {142, 108, 45}, {196, 172, 0},
    {95, 54, 80}, {128, 76, 255}, {201, 57, 1}, {246, 0, 122},
    {191, 162, 208}
};

int parse_path(const std::string & path, std::unordered_map<std::string, std::string> & result_map) noexcept {
    static std::vector<std::string> fields{"path", "dirname", "basename", "stem", "suffix"};
    static std::regex pattern{"^(.*)/((.*)\\.(.*))"};
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            result_map["path"] = path;
            result_map["dirname"] = path;
            return 0;
        } else {
            std::smatch results;
            if (std::regex_match(path, results, pattern)) {
                for (int i = 0; i < (int)fields.size(); i++) {
                    auto & key = fields[i];
                    result_map[key] = results[i];
                }
                return 0;
            } else {
                return -1;
            }
        }
    } else {
        return -1;
    }
}


int check_dir(const std::string & check_path, const bool is_mkdir) noexcept {
    int ret;
    struct stat st;
    if (stat(check_path.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;
        } else {
            return -1;
        }
    } else {
        if (is_mkdir) {
            ret = mkdir(check_path.c_str(), 00700);
            return ret;
        } else {
            return -1;
        }
    }
}


int check_file(const std::string & check_path, 
               std::vector<std::string> * file_set, 
               const std::vector<std::string> suffix) noexcept {
    struct stat st;
    char buf[BUFSIZ] = {0};
    if (stat(check_path.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            size_t total_file = 0;
            if (suffix.size()) {
                for (auto sfx: suffix) {
                    std::snprintf(buf, sizeof(buf), "%s%s%s", check_path.c_str(), "/*.", sfx.c_str());
                    glob_t gl;
                    int ret = glob(buf, GLOB_ERR, nullptr, &gl);
                    // if (ret != 0 || ret != GLOB_NOMATCH) {
                    //     return -2;
                    // }
                    for (size_t i = 0; i < gl.gl_pathc; i++) {
                        total_file ++;
                        const char * subpath = gl.gl_pathv[i];
                        if (file_set != nullptr) {
                            file_set->emplace_back(subpath);
                        }
                    }
                    globfree(&gl);
                }
            } else {
                if (file_set != nullptr) {
                    file_set->emplace_back(check_path);
                }
            }
            return 1 + total_file;
        } else if (S_ISREG(st.st_mode)) {
            if (file_set != nullptr) {
                file_set->emplace_back(check_path);
            }
            return 0;
        }
    } else {
        return -1;
    }
    return 0;
}