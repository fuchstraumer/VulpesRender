#pragma once
#ifndef VULPES_VK_INSTANCE_CONFIGURATION_H
#define VULPES_VK_INSTANCE_CONFIGURATION_H

#include "vpr_stdafx.h"

namespace vulpes {

	// Default width/height of window. Should probably move this elsewhere and/or remove it entirely.
	constexpr uint32_t DEFAULT_WIDTH = 1440, DEFAULT_HEIGHT = 900;

    namespace cfg {

	    enum class cameraType {
		    FPS,
		    FREE,
		    ARCBALL
        };

        struct vulpesInstanceInfo {

            /*
                These values don't affect engine behavior. ApplicationName sets
                the name of the created window, however.
            */
            std::string ApplicationName = std::string("DefaultAppName");
            std::string EngineName = std::string("VulpesRender");
            uint32_t ApplicationVersion = VK_MAKE_VERSION(0,1,0);
            uint32_t EngineVersion = VK_MAKE_VERSION(0,1,0);

            /*
                EnableValidation enables the Vulkan validation layers, set as 
                an instance-wide property. EnableMarkers enables debug markers,
                which can be used to clarify things in RenderDoc
            */
            bool EnableValidation;
            bool EnableMarkers = true;

            /*
                Fullscreen requires enabling the relevant Vulkan KHR extension,
                which isn't yet fully supported. 
            */
            bool EnableFullscreen = false;
            bool DefaultFullscreen = false;

            /*
                The window can be resized at runtime, but this sets the size of the initial window popup.
            */
            VkRect2D DefaultWindowSize = VkRect2D{ VkOffset2D{ 0, 0 }, VkExtent2D{ DEFAULT_WIDTH, DEFAULT_HEIGHT } };

            /*
                FPS camera locks pitch and doesn't allow roll. Free camera is fully unlocked and allows roll.
                Arcball camera is like a modelling program camera. Controls are documented in the relevant headers.
            */
            cameraType CameraType = cameraType::FPS;

            /*
                Whether or not to enable multisampling. Updating this option requires rebuilding the swapchain.
                SampleCount can max out at 64, or can be a slow as 1. Changing this still requires rebuilding the swapchain.
            */
            bool EnableMSAA = true;
            VkSampleCountFlagBits MSAA_SampleCount = VK_SAMPLE_COUNT_4_BIT;

            /*
                Texture anisotropy is not supported for all txture formats/types, but can still be safely set to true.
            */
            bool TextureAnisotropy = false;
            VkSampleCountFlagBits AnisotropySamples = VK_SAMPLE_COUNT_1_BIT;

            /*
                When disabled, the mouse is never locked into the screen's area. When enabled, the cursor
                can be broken free by holding LALT
            */
            bool EnableMouseLocking = true;

            float MouseSensitivity = 0.1f;
            float MovementSpeed = 25.0f;
	    };

    }

}


#endif //!VULPES_VK_INSTANCE_CONFIGURATION_H