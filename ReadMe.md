# VulpesRender
#### Or, yet another Vulkan renderer/framework

This project has been developed as part of a "learning" Vulkan exercise, but now has seen use 
in 2 personal projects and a commercial project in my workplace (getting paid to work on my
project, yay!). It attempts to use RAII (where appropriate) to simplify and ease interacting
with the Vulkan API. Elsewhere, like in the allocator subsystem, RAII is not used and memory 
allocations are performed infrequently: instead, a resource (like a buffer or image) requests
memory and the allocator returns a region for the resource to bind to and use like it's own
personal memory object. This increases performance, and can save GPU memory resources.

ImGui is currently lightly hooked in as the GUI of choice. All dependencies will be downloaded
and build after using CMake. These include:

- GLFW for windowing and input system management
- glslang for the WIP SPIR-V shader compiler
- assimp for importing models: defaults to only supporting OBJ and STL imports, currently

The BaseScene class is a highly virtual (virtualized?) class that takes care of setting up many of
the common resources that don't change between scenes, and contains usable methods for a rendering
loop and Vulkan command buffer submission. The following methods must be overridden:

- WindowResized(): initial notification of resize event. Use this to destroy all created resources and Vulkan objects, beyond what the BaseScene makes for you
- RecreatObjects(): self-explanatory. After recreation of the Vulkan backing resources (swapchain, framebuffers, renderpass, allocator, etc) recreate your objects.
- RecordCommands(): todo is a demo of this. Record Vulkan commands into command buffers here, and update ImGui.
- endFrame(): complete end-of-frame tasks for your personal resources. Not strictly required, and can be safely made into an empty method.

There are a considerable amount of items on my TODO list. The list below may be inaccurate or out of date, but reflects priorities:

- [ ] Get threaded transfer task pool functioning - DELAYED. MSVC std::promised_task broken, needs fix
- [x] In line with above, make Allocator subsystem and Buffer objects thread-safe. Synchronize mapping of VkDeviceMemory objects. (requires testing)
- [ ] Build a demo suite capable of testing most library items, and for demonstrating usage of the library
- [ ] Read/Write configuration values to a file and use the VulpesConfig struct more often
- [ ] Improve how descriptor pools are handled: currently have to guess max sets and required resource count upon first init call. Maybe build "requests"?
- [ ] Recover from ValidateMemory() errors

Example code of this library in use can be found in my DiamondDogs project, which is updated relatively frequently. The two primary scenes are:

Star rendering with GPU-noise texturing used for star surface and corona: found in scenes/StarScene.hpp
Terrain LOD experiments, using an adapted scheme from Proland: found in scenes/Terrain.hpp