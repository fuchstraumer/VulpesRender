#ifndef VULPES_VK_SHADER_COMPILER_H
#define VULPES_VK_SHADER_COMPILER_H

#include "vpr_stdafx.h"

#include "SPIRV/GlslangToSpv.h"
#include "glslang/Public/ShaderLang.h"
#include "SPIRV/disassemble.h"

namespace vulpes {

	namespace util {

		using spir_v_code_type = uint32_t;
		using spir_v_code_vector = std::vector<uint32_t>;

		class ShaderCompiler {
		public:

			ShaderCompiler();

			~ShaderCompiler();

			static std::tuple<bool, spir_v_code_vector> CompileShader(const std::string& source_code);

		private:

			void initializeGLSLang();
			void finalizeGLSLang();

			// avoids cost of opening a file to read an included glsl file.
			std::unordered_map<std::string, std::string> includeCache;
		};

	}

}

#endif //!VULPES_VK_SHADER_COMPILER_H