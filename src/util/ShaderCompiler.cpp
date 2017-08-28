#include "vpr_stdafx.h"
#include "util/ShaderCompiler.hpp"

namespace vulpes {

	namespace util {

		static constexpr TBuiltInResource default_resource_limits {
			/* .MaxLights = */ 32,
			/* .MaxClipPlanes = */ 6,
			/* .MaxTextureUnits = */ 32,
			/* .MaxTextureCoords = */ 32,
			/* .MaxVertexAttribs = */ 64,
			/* .MaxVertexUniformComponents = */ 4096,
			/* .MaxVaryingFloats = */ 64,
			/* .MaxVertexTextureImageUnits = */ 32,
			/* .MaxCombinedTextureImageUnits = */ 80,
			/* .MaxTextureImageUnits = */ 32,
			/* .MaxFragmentUniformComponents = */ 4096,
			/* .MaxDrawBuffers = */ 32,
			/* .MaxVertexUniformVectors = */ 128,
			/* .MaxVaryingVectors = */ 8,
			/* .MaxFragmentUniformVectors = */ 16,
			/* .MaxVertexOutputVectors = */ 16,
			/* .MaxFragmentInputVectors = */ 15,
			/* .MinProgramTexelOffset = */ -8,
			/* .MaxProgramTexelOffset = */ 7,
			/* .MaxClipDistances = */ 8,
			/* .MaxComputeWorkGroupCountX = */ 65535,
			/* .MaxComputeWorkGroupCountY = */ 65535,
			/* .MaxComputeWorkGroupCountZ = */ 65535,
			/* .MaxComputeWorkGroupSizeX = */ 1024,
			/* .MaxComputeWorkGroupSizeY = */ 1024,
			/* .MaxComputeWorkGroupSizeZ = */ 64,
			/* .MaxComputeUniformComponents = */ 1024,
			/* .MaxComputeTextureImageUnits = */ 16,
			/* .MaxComputeImageUniforms = */ 8,
			/* .MaxComputeAtomicCounters = */ 8,
			/* .MaxComputeAtomicCounterBuffers = */ 1,
			/* .MaxVaryingComponents = */ 60,
			/* .MaxVertexOutputComponents = */ 64,
			/* .MaxGeometryInputComponents = */ 64,
			/* .MaxGeometryOutputComponents = */ 128,
			/* .MaxFragmentInputComponents = */ 128,
			/* .MaxImageUnits = */ 8,
			/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
			/* .MaxCombinedShaderOutputResources = */ 8,
			/* .MaxImageSamples = */ 0,
			/* .MaxVertexImageUniforms = */ 0,
			/* .MaxTessControlImageUniforms = */ 0,
			/* .MaxTessEvaluationImageUniforms = */ 0,
			/* .MaxGeometryImageUniforms = */ 0,
			/* .MaxFragmentImageUniforms = */ 8,
			/* .MaxCombinedImageUniforms = */ 8,
			/* .MaxGeometryTextureImageUnits = */ 16,
			/* .MaxGeometryOutputVertices = */ 256,
			/* .MaxGeometryTotalOutputComponents = */ 1024,
			/* .MaxGeometryUniformComponents = */ 1024,
			/* .MaxGeometryVaryingComponents = */ 64,
			/* .MaxTessControlInputComponents = */ 128,
			/* .MaxTessControlOutputComponents = */ 128,
			/* .MaxTessControlTextureImageUnits = */ 16,
			/* .MaxTessControlUniformComponents = */ 1024,
			/* .MaxTessControlTotalOutputComponents = */ 4096,
			/* .MaxTessEvaluationInputComponents = */ 128,
			/* .MaxTessEvaluationOutputComponents = */ 128,
			/* .MaxTessEvaluationTextureImageUnits = */ 16,
			/* .MaxTessEvaluationUniformComponents = */ 1024,
			/* .MaxTessPatchComponents = */ 120,
			/* .MaxPatchVertices = */ 32,
			/* .MaxTessGenLevel = */ 64,
			/* .MaxViewports = */ 16,
			/* .MaxVertexAtomicCounters = */ 0,
			/* .MaxTessControlAtomicCounters = */ 0,
			/* .MaxTessEvaluationAtomicCounters = */ 0,
			/* .MaxGeometryAtomicCounters = */ 0,
			/* .MaxFragmentAtomicCounters = */ 8,
			/* .MaxCombinedAtomicCounters = */ 8,
			/* .MaxAtomicCounterBindings = */ 1,
			/* .MaxVertexAtomicCounterBuffers = */ 0,
			/* .MaxTessControlAtomicCounterBuffers = */ 0,
			/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
			/* .MaxGeometryAtomicCounterBuffers = */ 0,
			/* .MaxFragmentAtomicCounterBuffers = */ 1,
			/* .MaxCombinedAtomicCounterBuffers = */ 1,
			/* .MaxAtomicCounterBufferSize = */ 16384,
			/* .MaxTransformFeedbackBuffers = */ 4,
			/* .MaxTransformFeedbackInterleavedComponents = */ 64,
			/* .MaxCullDistances = */ 8,
			/* .MaxCombinedClipAndCullDistances = */ 8,
			/* .MaxSamples = */ 4,
			/* .limits = */
			{
			/* .nonInductiveForLoops = */ 1,
			/* .whileLoops = */ 1,
			/* .doWhileLoops = */ 1,
			/* .generalUniformIndexing = */ 1,
			/* .generalAttributeMatrixVectorIndexing = */ 1,
			/* .generalVaryingIndexing = */ 1,
			/* .generalSamplerIndexing = */ 1,
			/* .generalVariableIndexing = */ 1,
			/* .generalConstantMatrixVectorIndexing = */ 1,
			}	 
		};

		static const std::vector<std::string> include_directories{
			std::string("rsrc/shaders/"),
			std::string("C:/Users/kgstr/Documents/Visual Studio 2017/Projects/DiamondDogs/rsrc/shaders/")
		};

		static void process_includes(std::string& input) {
			static const std::basic_regex<char> re("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
			std::stringstream input_stream;
			input_stream << input;
			std::smatch matches;
			// Current line
			std::string line;
			while (std::getline(input_stream, line)) {
				if (std::regex_search(line, matches, re)) {
					std::string include_file = matches[1];
					std::string include_string;
					std::ifstream include_stream;
					// Try to open included file.
					try {
						std::string path = "rsrc/shaders/include/" + include_file;
						include_stream.open(path);
						std::stringstream tmp;
						tmp << include_stream.rdbuf();
						tmp << "\n";
						include_string = tmp.str();
						include_stream.close();
					}
					catch (std::ifstream::failure e) {
						std::cerr << "ERROR::SHADER::INCLUDED_FILE_NOT_SUCCESFULLY_READ: " << include_file << std::endl;
					}
					input = std::regex_replace(input, re, include_string);
				}
			}
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
			throw std::runtime_error("Failed to find shader stage using file extension.");
		}

		static std::tuple<std::string, uint32_t> OpenFile(const std::string& full_file_path) {

			uint32_t code_size = 0;
			std::string result;

			try {
				std::ifstream input(full_file_path, std::ios::binary | std::ios::in | std::ios::ate);
				input.exceptions(std::ios::failbit | std::ios::badbit);
				code_size = static_cast<uint32_t>(input.tellg());
				assert(code_size > 0);
				result.resize(code_size);
				input.seekg(0, std::ios::beg);
				std::stringstream str_stream;
				str_stream << input.rdbuf();
				str_stream << "\n";
				result = str_stream.str();
				input.close();

			}
			catch (std::ifstream::failure&) {
				LOG(ERROR) << "Failed to open/read given shader file.";
				throw(std::runtime_error("Failure opening or reading shader file."));
			}

			return std::make_tuple(result, code_size);
		}

		static void DumpBadShaderLog(glslang::TShader* bad_shader, const std::string& shader_code) {
			std::ofstream shader_failure_log;
			std::string filename = std::string("logs/shader_failure_") + std::to_string(reinterpret_cast<uint32_t>(bad_shader)) + std::string(".txt");
			shader_failure_log.open(filename.c_str(), std::ios::out);
			if (shader_failure_log.good()) {
				shader_failure_log << "Shader code: \n";
				shader_failure_log << shader_code.c_str();
				shader_failure_log << "Shader info log: \n";
				shader_failure_log << bad_shader->getInfoLog() << "\n";
				shader_failure_log << bad_shader->getInfoDebugLog() << "\n";
				shader_failure_log.close();
			}
			else {
				throw std::runtime_error("Couldn't open output log dump.");
			}
		}

		static void DumpBadProgramLog(glslang::TProgram* bad_program) {
			std::ofstream program_failure_log;
			std::string filename = std::string("logs/program_failure_") + std::to_string(reinterpret_cast<uint32_t>(bad_program)) + std::string(".txt");
			program_failure_log.open(filename.c_str(), std::ios::out);
			if (program_failure_log.good()) {
				program_failure_log << "Shader info log: \n";
				program_failure_log << bad_program->getInfoLog() << "\n";
				program_failure_log << bad_program->getInfoDebugLog() << "\n";
				program_failure_log.close();
			}
			else {
				throw std::runtime_error("Couldn't open output log dump.");
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
			glslang::TShader::ForbidIncluder includer;

			EProfile profile = ECoreProfile;
			EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);
			int default_version = 450;

			auto shader_source = OpenFile(filename);
			std::vector<std::string> includes;
			std::string& shader_source_ref = std::get<0>(shader_source);

			process_includes(shader_source_ref);

			// add required padding.
			size_t padding = shader_source_ref.size() % 4;
			if (padding != 0) {
				shader_source_ref.append(4 - padding, '\n');
			}

			// attach source to shader object.
			const char* shader_src_c_str = std::get<0>(shader_source).c_str();
			const int shader_src_length = static_cast<int>(shader_source_ref.size());
			shader->setStringsWithLengths(&shader_src_c_str, &shader_src_length, 1);

			// perform parsing run.
			if (!shader->parse(&default_resource_limits, default_version, false, messages, includer)) {
				DumpBadShaderLog(shader.get(), shader_source_ref);
				throw std::runtime_error("Failed to parse shader! Check logfile for dumped errors.");
			}

			// Link shader to form program (despite only a single stage, still has to be made into a program
			std::unique_ptr<glslang::TProgram> program = std::make_unique<glslang::TProgram>();
			program->addShader(shader.get());
			if (!program->link(messages)) {
				DumpBadProgramLog(program.get());
				throw std::runtime_error("Failed to link shader into a program!");
			}

			// Generate intermediate AST representation.
			glslang::TIntermediate* intermediate = program->getIntermediate(shader_stage);
			if (!intermediate) {
				DumpBadProgramLog(program.get());
				throw std::runtime_error("Failed to retrieve/generate intermdiary AST representation for a shader!");
			}

			// perform final build step.
			std::vector<uint32_t> result_vector;
			spv::SpvBuildLogger logger;
			glslang::GlslangToSpv(*intermediate, result_vector, &logger);

			if (strlen(shader->getInfoLog())) {
				LOG(ERROR) << "Shader info log dump: " << shader->getInfoLog();
			}

			if (strlen(shader->getInfoDebugLog())) {
				LOG(ERROR) << "Shader debug info log dump: " << shader->getInfoDebugLog();
			}

			if (strlen(program->getInfoLog())) {
				LOG(ERROR) << "Program info log dump: " << program->getInfoLog() << "\n";
			}

			if (strlen(program->getInfoDebugLog())) {
				LOG(ERROR) << "Program debug info log dump: " << program->getInfoDebugLog();
			}

			auto spirv_messages = logger.getAllMessages();
			if (!spirv_messages.empty()) {
				LOG(ERROR) << "SPIR-V compiler log dump: " << spirv_messages.c_str();
			}

			return result_vector;
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


