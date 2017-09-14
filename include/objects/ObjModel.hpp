#ifndef VULPESRENDER_OBJ_MODEL_HPP
#define VULPESRENDER_OBJ_MODEL_HPP

#include "vpr_stdafx.h"
#include "TriangleMesh.hpp"
#include "resource/Texture.hpp"
#include "resource/Buffer.hpp"
#include "resource/ShaderModule.hpp"
#include "resource/DescriptorSet.hpp"
#include "resource/PipelineLayout.hpp"
#include "resource/PipelineCache.hpp"
#include "render/GraphicsPipeline.hpp"


namespace vulpes {

    class ObjModel : public TriangleMesh {
        ObjModel(const ObjModel&) = delete;
        ObjModel& operator=(const ObjModel&) = delete;
    public:

        ObjModel(const Device* dvc);
        ~ObjModel();

        void LoadFromFile(const std::string& obj_model_filename);

    };

}


#endif //!VULPESRENDER_OBJ_MODEL_HPP