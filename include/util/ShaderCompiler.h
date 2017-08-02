#ifndef VULPES_VK_SHADER_COMPILER_H
#define VULPES_VK_SHADER_COMPILER_H

#include "vpr_stdafx.h"

#include "SPIRV/GlslangToSpv.h"
#include "glslang/Public/ShaderLang.h"
#include "SPIRV/disassemble.h"

namespace vulpes {

	namespace util {

		class ShaderCompiler {
		public:

			ShaderCompiler();

			~ShaderCompiler();
			
			std::vector<uint32_t> CompileShader(const std::string& source_code);

		private:

			std::vector<uint32_t> compileShaderToSpirV(const EShLanguage& stage, const std::string& source_code);

			void initializeGLSLang();
			void finalizeGLSLang();

			// avoids cost of opening a file to read an included glsl file.
			std::unordered_map<std::string, std::string> includeCache;
		};

	}

}

#endif //!VULPES_VK_SHADER_COMPILER_H