#pragma once
#ifndef VULPES_VK_SCENE_CONFIGURATION_H
#define VULPES_VK_SCENE_CONFIGURATION_H

#include "vpr_stdafx.h"

namespace vulpes {

	// Default width/height of window. Should probably move this elsewhere and/or remove it entirely.
	constexpr uint32_t DEFAULT_WIDTH = 1440, DEFAULT_HEIGHT = 900;

	    enum class cameraType : int {
		    FPS = 0,
		    ARCBALL
        };

        struct vulpesSceneConfig {

            /*
                These values don't affect engine behavior. ApplicationName sets
                the name of the created window, however.
            */
            std::string ApplicationName = std::string("DefaultAppName");
            std::string EngineName = std::string("VulpesRender");
            uint32_t ApplicationVersion = VK_MAKE_VERSION(0,1,0);
            uint32_t EngineVersion = VK_MAKE_VERSION(0,1,0);

            /*
                Prepended to all internal resource paths used for the common objects,
                so that the default shaders can be safely loaded. Default value might
                fail, or cause a relatively expensive recursive search to trigger.
            */
            std::string ResourcePathStr = std::string("./");

            /*
                EnableValidation enables the Vulkan validation layers, set as 
                an instance-wide property. EnableMarkers enables debug markers,
                which can be used to clarify things in RenderDoc
            */
            bool EnableValidation = false;
            bool EnableMarkers = true;

            /*
                Fullscreen requires enabling the relevant Vulkan KHR extension,
                which isn't yet fully supported. 
            */
            bool EnableFullscreen = false;
            bool DefaultFullscreen = false;

            /*
                Whether or not to enable the GUI. If disabled, it can't be added during runtime as this
                relates to important background functionality required for the GUI to work properly. 
            */
            bool EnableGUI = true;
            
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
            VkSampleCountFlagBits MSAA_SampleCount = VK_SAMPLE_COUNT_8_BIT;

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

            /*
                Frame limiting. Enabled by default. Time can be modified too: currently locked to 60fps max.
            */
            bool LimitFramerate = true;
            float FrameTimeMs = 16.0f;

            /*
                Movement speed affects camera speed in FPS/FREE camera modes.
            */
            float MouseSensitivity = 0.5f;
            float MovementSpeed = 25.0f;

            /*
                Enable the usage of 3D mouse picking by using data read back from the renderpass. Writes data about primitives
                under the mouse if enabled.
            */
            bool Enable3DMousePicking = true;

			bool RequestRefresh = false;

			/*
			
				Verbose logging includes several extra info log calls at various locations. This is useful 
				for tracking where crashes occur in release builds. 

			*/

			bool VerboseLogging = true;

	    };

    

}


#endif //!VULPES_VK_SCENE_CONFIGURATION_H