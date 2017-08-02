#include "ShaderCompiler.h"
#include <regex>
namespace vulpes {

	namespace util {

		static std::vector<std::string> find_includes(std::string& input_code) {

			std::vector<std::string> result;

			static const std::basic_regex<char> include_search("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
			std::stringstream input_stream;
			input_stream << input_code;
			std::smatch matches;

			std::string curr_line;

			while (std::getline(input_stream, curr_line)) {
				if (std::regex_search(curr_line, matches, include_search)) {
					std::string include_file = matches[1];
					result.push_back(include_file);
					input_code.erase(matches.position(), matches.position() + matches.length() + 1);
				}
			}

			return result;

		}

		ShaderCompiler::ShaderCompiler() {
			bool init_glslang = initializeGLSLang();
		}

		ShaderCompiler::~ShaderCompiler()
		{
		}

		void ShaderCompiler::initializeGLSLang() {

			if(!glslang::InitializeProcess()) {
				LOG(ERROR) << "Failed to initialize GLSLang process!\n";
				throw std::runtime_error("Failed to init GLSLang");
			}

		}

		void ShaderCompiler::finalizeGLSLang() {
			return false;
		}

	}

}


