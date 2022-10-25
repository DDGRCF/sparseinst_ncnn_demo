#ifndef COMMON_H_
#define COMMON_H_

#include <vector>
#include <string>
#include <unordered_map>

extern const std::vector<std::vector<int>> color_list;


int parse_path(const std::string & path, std::unordered_map<std::string, std::string> & result_map) noexcept;

int check_dir(const std::string & check_path, const bool is_mkdir) noexcept;

int check_file(const std::string & check_path, 
               std::vector<std::string> * file_set, 
               const std::vector<std::string> suffix) noexcept;

#endif