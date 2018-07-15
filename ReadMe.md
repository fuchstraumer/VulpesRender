# VulpesRender
#### Or, yet another Vulkan wrapper

This project has been recently restructured: if you want to use the old style, check for tags before v0.9. As of v0.9, what was previously a folder has become a unique target - which will default build to a DLL.

Current targets are:
- `vpr_core`: For `Instance`, `Device`, `Swapchain`, `SurfaceKHR`, and `PhysicalDevice`
- `vpr_alloc`: For creation of an `Allocator`, `Allocation`s, and usage of `AllocationRequirements` as needed
- `vpr_resource`: `Buffer`, `Image`, `DescriptorSet`, `DescriptorPool`, `DescriptorSetLayout`, `PipelineLayout`, `PipelineCache`, `ShaderModule`, and `Sampler`. I don't recommend using the Image/Buffer classes as they are no longer maintained. 
- `vpr_render`: `Renderpass`, `Framebuffer`, and `GraphicsPipeline`. Also no longer maintained.
- `vpr_sync`: `Event`, `Semaphore`, and `Fence`. Maintained but incredibly simple, `Event` is the most complex with member functions but the rest are just `VkSemaphore` and `VkFence` given RAII wrappers.

I haven't figured out a good way to get CMake to copy DLLs to a client executables location yet though, so you'll have to do this yourself before things work. Always interested to hear about potential better ways to do this, though.

### Why the new design?

I found myself using the abstract classes in `vpr_resource` and `vpr_render` less and less: my new data-driven rendering work does enough of the work that using these objects wasn't worth it. In particular, I previously tied a `vpr::Allocator` object to a single `vpr::Device`: when in fact, I may end up with multiple allocators (like one per thread). I also did a lot of lifetime management through the allocator interface, and the way I had Buffers/Images setup made delayed initialization or sharing of Buffer/Image handles (e.g, duplicate copies using same API handle) very difficult.

`vpr_core` is then me paring things down to the bare minimum of what one needs to create a Vulkan application. The core functionality is quite useful, and extremely easy to use - though you'll need to create your own window class ([example here](https://gist.github.com/fuchstraumer/9055fe7a4a0cfc1f8ebc598fa162fa85)) and input system (no examples for this: mine is still up in the air, and I don't have any good answers on how to do it optimally!)

### Questions/Comments?

I'm always interested in improving my libraries and code! If you have any questions, comments, or criticism feel free to create an issue outlining them. If you're new to the Vulkan API and looking at designing your own Vulkan abstraction and want help, feel free to ask via an issue as well! We can take it to other, more private/thorough, contact methods from there
