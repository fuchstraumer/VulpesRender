#pragma once
#ifndef VULPESRENDER_GRAPHICS_SETTINGS_HPP
#define VULPESRENDER_GRAPHICS_SETTINGS_HPP
#include <vulkan/vulkan.h>

namespace vpr {

    struct vulpes_graphics_options_t {

        /*
            These values don't affect engine behavior. ApplicationName sets
            the name of the created window, however.
        */
        std::string ApplicationName = std::string("DefaultAppName");
        std::string EngineName = std::string("VulpesRender");
        uint32_t ApplicationVersion = VK_MAKE_VERSION(0,1,0);
        uint32_t EngineVersion = VK_MAKE_VERSION(0,1,0);

        constexpr static uint32_t DEFAULT_WIDTH = 1440;
        constexpr static uint32_t DEFAULT_HEIGHT = 720;

        bool EnableValidation = false;
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

    struct vulpes_state_t {
        bool ShouldMouseLock = false;
        bool NeedWindowResize = false;
    };

}

#endif //!VULPESRENDER_GRAPHICS_SETTINGS_HPP