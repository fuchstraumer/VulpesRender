#include "vpr_stdafx.h"
#include <unordered_map>
#include "objects/ObjModel.hpp"


#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace vulpes {

    void ObjModel::LoadModelFromFile(const std::string& obj_model_filename) {
        
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string tinyobj_err;

        if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &tinyobj_err, obj_model_filename.c_str())) {
            LOG(ERROR) << "TinyObjLoader failed to load model file " << obj_model_filename << " , exiting.";
            LOG(ERROR) << "Load failed with error: " << tinyobj_err;
            throw std::runtime_error(tinyobj_err.c_str());
        }

        if(!materials.empty()) {
            createTextures(materials);
        }
        
    }
    
    void ObjModel::createTextures(const std::vector<tinyobj::material_t>& materials) {
        
    }

}