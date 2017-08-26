#ifndef VULPES_VK_SHADER_COMPILER_H
#define VULPES_VK_SHADER_COMPILER_H

#include "vpr_stdafx.h"



namespace vulpes {

	namespace util {

		class ShaderCompiler {
		public:

			ShaderCompiler();

			~ShaderCompiler();
			
			std::vector<uint32_t> CompileShader(const std::string& filename);

		private:

			std::vector<uint32_t> compileShaderToSpirV(const EShLanguage& stage, const std::string& source_code);

			void initializeGLSLang();
			void finalizeGLSLang();

		};

	}

}

#endif //!VULPES_VK_SHADER_COMPILER_H