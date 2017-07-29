#pragma once
#ifndef VULPES_VK_INSTANCE_CONFIGURATION_H
#define VULPES_VK_INSTANCE_CONFIGURATION_H

#include "vpr_stdafx.h"

namespace vulpes {

    namespace cfg {

        
	    enum class cameraType {
		    FPS,
		    FREE,
		    ARCBALL
        };

        struct vulpesInstanceInfo {
            std::string ApplicationName = std::string("DefaultAppName"), EngineName = std::string("DefaultEngineName");
            uint32_t ApplicationVersion = VK_MAKE_VERSION(0,1,0), EngineVersion = VK_MAKE_VERSION(0,1,0), apiVersion = VK_API_VERSION_1_0;
            bool EnableValidation;
            bool EnableMarkers = true;
            bool EnableFullscreen = false;
            bool DefaultFullscreen = false;
            VkRect2D DefaultWindowSize = VkRect2D{ VkOffset2D{}, VkExtent2D{ 1440, 900 } };
            cameraType CameraType = cameraType::FPS;
            bool EnableMSAA = true;
            VkSampleCountFlagBits MSAA_SampleCount = VK_SAMPLE_COUNT_4_BIT;
            bool EnableHDR = false;
            bool EnableBloom = false;
            bool TextureAnisotropy = false;
            VkSampleCountFlagBits AnisotropySamples = VK_SAMPLE_COUNT_1_BIT;
            bool EnableMouseLocking = false;
            float MouseSensitivity = 0.2f;
            float MovementSpeed = 25.0f;
	    };

    }

}


#endif //!VULPES_VK_INSTANCE_CONFIGURATION_H