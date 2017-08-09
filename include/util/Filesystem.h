#ifndef VULPES_FILESYSTEM_UTILITIES_H
#define VULPES_FILESYSTEM_UTILITIES_H

#include "vpr_stdafx.h"

namespace vulpes {

    namespace util {

        using fs_path = std::experimental::filesystem::path;
        using fs_status = std::experimental::filesystem::file_status;
        using fs_entry = std::experimental::filesystem::directory_entry;
        using file_info = std::tuple<fs_path, fs_status, size_t>;

        inline static file_info getFileInfo(const fs_entry& entry) {
            const fs_status _fs_status(std::experimental::filesystem::status(entry));
            size_t file_size = 0;
            if(std::experimental::filesystem::is_regular_file(entry.path())) {
                file_size = std::experimental::filesystem::file_size(entry.path());
            }
            return { entry.path(), _fs_status, size_t(file_size) };
        }

        static char type_symbol(const fs_status& fs) {
            if(std::experimental::filesystem::is_directory(fs)) {
                return 'd';
            }
            else if(std::experimental::filesystem::is_regular_file(fs)) {
                return 'f';
            }
            else if(std::experimental::filesystem::is_symlink(fs)) {
                return 'l';
            }
            else if(std::experimental::filesystem::is_block_file(fs)) {
                return 'b';
            }
            else if(std::experimental::filesystem::is_fifo(fs)) {
                return 'f';
            }
            else if(std::experimental::filesystem::is_socket(fs)) {
                return 's';
            }
            else {
                return '?';
            }
        }

        static std::string getPermissions(const std::experimental::filesystem::perms& permission) {
            
            auto check = [permission](const std::experimental::filesystem::perms& bit, const char& c)->char {
                return (permission & bit) == std::experimental::filesystem::perms::none ? '-' : c;
            };

            // Build permissions string.
            std::string result { 
                check(std::experimental::filesystem::perms::owner_read, 'r'),
                check(std::experimental::filesystem::perms::owner_write, 'w'),
                check(std::experimental::filesystem::perms::owner_exec, 'x'),
                check(std::experimental::filesystem::perms::group_read, 'r'),
                check(std::experimental::filesystem::perms::group_write, 'w'),
                check(std::experimental::filesystem::perms::group_exec, 'x'),
                check(std::experimental::filesystem::perms::others_read, 'r'),
                check(std::experimental::filesystem::perms::others_write, 'w'),
                check(std::experimental::filesystem::perms::others_exec, 'x'),
            };

            return result;
        }

        static std::string getSizeString(const size_t& size) {
           
            std::stringstream s_str;
            
            if(size >= 1e9) {
                s_str << (static_cast<double>(size) / 1.0e9) << 'G';
            }
            else if(size >= 1e6) {
                s_str << (static_cast<double>(size) / 1.0e6) << 'M';
            }
            else if(size >= 1e3) {
                s_str << (static_cast<double>(size) / 1.0e3) << 'K';
            }
            else {
                s_str << static_cast<double>(size) << 'B';
            }

            return s_str.str();

        }

        std::vector<file_info> ListFilesInDirectory(const std::string& dir = ".") {
            
            if(!std::experimental::filesystem::exists({ dir })) {
                return std::vector<file_info>();
            }

            std::vector<file_info> results;

            // for each result from the directory iterator, use getFileInfo() and insert it into the back of results.
            std::transform(std::experimental::filesystem::directory_iterator(std::experimental::filesystem::path(dir)), {}, std::back_inserter(results), getFileInfo);

            return results;
        }

    }

}

#endif //!VULPES_FILESYSTEM_UTILITIES_H