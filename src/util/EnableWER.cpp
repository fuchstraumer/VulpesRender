#include "vpr_stdafx.h"
#include "util/EnableWER.hpp"
#include "core/Instance.hpp"
namespace vulpes {
    namespace util {

        inline static void winErrorToLog(const LSTATUS& status) {
            
            if (status == ERROR_ACCESS_DENIED) {
                LOG(ERROR) << "Windows API call returned ERROR_ACCESS_DENIED:" << std::to_string(status);
            }
            else if (status == ERROR_PATH_NOT_FOUND) {
                LOG(ERROR) << "Windows API call returned ERROR_PATH_NOT_FOUND:" << std::to_string(status);;
            }
            else if (status == ERROR_INVALID_DATA) {
                LOG(ERROR) << "Windows API call returned ERROR_INVALID_DATA:" << std::to_string(status);
            }
            else {
                LOG(ERROR) << "Windows API call returned:" << std::to_string(status);
            }
        }

        void wer_enabler_t::enable() const {

            namespace fs = std::experimental::filesystem;
            fs::path curr_dir = fs::current_path();

            curr_dir /= ".tmp";

            if (!fs::exists(curr_dir)) {
                LOG(WARNING) << "Created folder .tmp in wer_enabler_t: should've been created already.";
                fs::create_directory(curr_dir);
            }

            curr_dir /= "crash_dumps";

            if (!fs::exists(curr_dir)) {
                LOG(INFO) << "Created crash dump folder in .tmp.";
                fs::create_directory(curr_dir);
            }

            std::string registry_key_name = "SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\";

            registry_key_name += Instance::VulpesInstanceConfig.ApplicationName;

            if (Instance::VulpesInstanceConfig.ApplicationName == "VulpesRender") {
                LOG(WARNING) << "ApplicationName in Config struct is still set to default value of VulpesRender: update this to ensure memory dump and error log is correctly setup.";
            }

            HKEY created_hkey;
            DWORD dw_disposition;
            
            auto open_error = RegCreateKeyEx(HKEY_LOCAL_MACHINE, registry_key_name.c_str(), 0, NULL, 0, KEY_READ, NULL, &created_hkey, &dw_disposition);
            if (open_error == ERROR_SUCCESS) {
                LOG(INFO) << "Required registry key for memory dumps already exists.";
                return;
            }

            auto win_error = RegCreateKeyEx(HKEY_LOCAL_MACHINE, registry_key_name.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &created_hkey, &dw_disposition);
            if (win_error == ERROR_SUCCESS) {
                DWORD dw_type, dw_size;
                dw_type = REG_DWORD;
                dw_size = sizeof(DWORD);
                auto dump_folder = curr_dir.string();
                DWORD32 dump_type = DWORD32(2);
                DWORD32 dump_count = DWORD32(5);

                // Set dump folder
                win_error = RegSetValueEx(created_hkey, TEXT("DumpFolder"), 0, REG_SZ, (BYTE*)(dump_folder.c_str()), ((DWORD)(dump_folder.size() * sizeof(char))));
                
                if (win_error != ERROR_SUCCESS) {
                    LOG(ERROR) << "Failed to set memory dump output folder registry value";
                    winErrorToLog(win_error);
                }

                // Set dump count
                win_error = RegSetValueEx(created_hkey, TEXT("DumpCount"), 0, REG_DWORD, (BYTE*)&dump_count, sizeof(dump_count));
               
                if (win_error != ERROR_SUCCESS) {
                    LOG(ERROR) << "Failed to set memory dump count registry value.";
                    winErrorToLog(win_error);
                }

                // Set dump type
                win_error = RegSetValueEx(created_hkey, TEXT("DumpType"), 0, REG_DWORD, (BYTE*)&dump_type, sizeof(dump_type));
                
                if (win_error != ERROR_SUCCESS) {
                    LOG(ERROR) << "Failed to set memory dump type registry value.";
                    winErrorToLog(win_error);
                }

                DWORD32 dump_flags = DWORD32(0);
                win_error = RegSetValueEx(created_hkey, TEXT("DumpType"), 0, REG_DWORD, (BYTE*)&dump_flags, sizeof(dump_flags));

                if (win_error != ERROR_SUCCESS) {
                    LOG(ERROR) << "Failed to set memory dump custom flags registry value.";
                    winErrorToLog(win_error);
                }

                LOG(INFO) << "Registry key to enable memory dumps created.";
            }
            else {
                winErrorToLog(win_error);
            }
        }


    }
}