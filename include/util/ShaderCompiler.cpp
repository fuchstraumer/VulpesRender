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

		static EShLanguage GetShaderStage(const std::string& filename) {
			size_t idx = filename.find_last_of(".");
			const std::string extension = filename.substr(idx + 1);
			if (extension == "vert") {
				return EShLangVertex;
			}
			if (extension == "tesc") {
				return EShLangTessControl;
			}
			if (extension == "tese") {
				return EShLangTessEvaluation;
			}
			if (extension == "geom") {
				return EShLangGeometry;
			}
			if (extension == "frag") {
				return EShLangFragment;
			}
			if (extension == "comp") {
				return EShLangCompute;
			}
		}

		ShaderCompiler::ShaderCompiler() {
			initializeGLSLang();
		}

		ShaderCompiler::~ShaderCompiler() {
			finalizeGLSLang();
		}

		std::vector<uint32_t> ShaderCompiler::CompileShader(const std::string & filename) {

			auto shader_stage = GetShaderStage(filename);

			std::unique_ptr<glslang::TShader> shader = std::make_unique<glslang::TShader>(shader_stage);
			std::unique_ptr<glslang::TProgram> program;
			glslang::TShader::ForbidIncluder includer;

			return std::vector<uint32_t>();
		}

		void ShaderCompiler::initializeGLSLang() {

			if(!glslang::InitializeProcess()) {
				LOG(ERROR) << "Failed to initialize GLSLang process!\n";
				throw std::runtime_error("Failed to init GLSLang");
			}

		}

		void ShaderCompiler::finalizeGLSLang() {
			glslang::FinalizeProcess();
		}

	}

}


