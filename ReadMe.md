# VulpesRender
#### Or, yet another Vulkan wrapper...

This project has been recently restructured: if you want to use the old style, check for tags before v0.9. As of v0.9, what was previously a folder has become a unique target - which will default build to a shared library.

Current targets are:
- `vpr_core`: For `Instance`, `Device`, `Swapchain`, `SurfaceKHR`, and `PhysicalDevice`
- `vpr_alloc`: For creation of an `Allocator`, `Allocation`s, and usage of `AllocationRequirements` as needed
- `vpr_resource`: `Buffer`, `Image`, `DescriptorSet`, `DescriptorPool`, `DescriptorSetLayout`, `PipelineLayout`, `PipelineCache`, `ShaderModule`, and `Sampler`. I don't recommend using the Image/Buffer classes as they are no longer maintained. 
- `vpr_render`: `Renderpass`, `Framebuffer`, and `GraphicsPipeline`. Also no longer maintained.
- `vpr_sync`: `Event`, `Semaphore`, and `Fence`. Maintained but incredibly simple, `Event` is the most complex with member functions but the rest are just `VkSemaphore` and `VkFence` given RAII wrappers.

I haven't figured out a good way to get CMake to copy DLLs to a client executables location yet though, so you'll have to do this yourself before things work. Always interested to hear about potential better ways to do this, though.

### Why the new design?

I found myself using the abstract classes in `vpr_resource` and `vpr_render` less and less: my new data-driven rendering work does enough of the work that using these objects wasn't worth it. In particular, I previously tied a `vpr::Allocator` object to a single `vpr::Device`: when in fact, I may end up with multiple allocators (like one per thread). I also did a lot of lifetime management through the allocator interface, and the way I had Buffers/Images setup made delayed initialization or sharing of Buffer/Image handles (e.g, duplicate copies using same API handle) very difficult.

`vpr_core` is then me paring things down to the bare minimum of what one needs to create a Vulkan application. The core functionality is quite useful, and extremely easy to use - though you'll need to create your own window class ([example here](https://gist.github.com/fuchstraumer/9055fe7a4a0cfc1f8ebc598fa162fa85)) and input system (no examples for this: mine is still up in the air, and I don't have any good answers on how to do it optimally!). Further examples of how I wrap the `vpr_core` objects up into a single interface an be found in Caelestis, in the `renderer_context` plugin [here](https://github.com/fuchstraumer/Caelestis/tree/a88d9184e259fc199e8f773ee95caefd6edd14ab/plugins/renderer_context). I don't actually use the Input handler class there: I found it easier to just use callback functions (i.e, delegates) and selectively subscribe to those as needed.

### Deprecated Resource Classes?

I have deprecated the `Buffer` and `Image` classes. This is for a couple of relatively complex reasons that I'm not able to succinctly explain here, but at it's core it relates to how I create resources in my upper-level libraries. Both classes made a lot of assumptions about how the user wanted to use them (such as automatically creating `VkBufferView`/`VkImageView` objects, and not allowing for more than one of those) and made assumptions about setup and transfer of initial data to these objects. It required calling a `ClearStagingBuffers()` function in end-user code, which is not great as it's easy to miss or forget about (and how do we know it's safe to call this? what if transfer are pending?). If you're interested in the system I used to replace these, you can find an article about my Vulkan resource system over [here](https://fuchstraumer.github.io/Vulkan-Resource-Plugin/). This resource system has ended up being a much more usable way of managing Buffer/Image resources, so I at least recommend looking into it if you're curious.

If you're just starting out with Vulkan and need to get things off the ground though, I think these classes will serve you alright. I might someday go back and polish them up to be usable for "singleton" use cases and the like, so hold tight on that.

### Dependencies

The primary dependencies, glfw and easyloggingpp, are included as submodules and will need to be cloned before building. If building to a shared library, make sure that `BUILD_SHARED_LIBS` is enabled as glfw behaves oddly when linked to statically then used across multiple shared libraries (unsurprisingly).

If you are using SDL instead of glfw, its header and library files must be in your compiler's include directory.

The dependency on `boost::variant` has been removed, as I'm now using the single header `variant-lite` that can be found as a submodule in `third_party/`. This was super easy to integrate, removed my need for a bunch of nasty preprocessor switches, and should help make builds more lightweight and easy since the build script doesn't need to drag in (or find!) a bunch of `boost` stuff now. Let me know if it doesn't work or acts oddly though!

### easyloggingpp and Shared Libraries

In order to get easyloggingpp to share a unified logging repository with some core application or runtime, call the functions used to bind to a new repository in each of the unique shared library targets here. These can be found in:

- for `vpr_core`, in `Instance.hpp` as `SetLoggingRepository_VprCore`
- for `vpr_alloc`, in `Allocator.hpp` as `SetLoggingRepository_VprAlloc`
- for `vpr_resource`, in `PipelineCache.hpp` as `SetLoggingRepository_VprResource`
- for `vpr_command`, in `CommandPool.hpp` as `SetLoggingRepository_VprCommand`
- not implemented for `vpr_sync` or `vpr_render`, as no logging occurs in these targets

### Swapchain Resizing

When performing a swapchain resize event, use the method in `Swapchain.hpp` called `RecreateSwapchainAndSurface` to make sure that both of the relevant objects are destroyed and recreated in the proper order. This method also calls required functions for checking for surface support, as without these the validation layers will throw some rather verbose warnings at you (which is a bit silly, admittedly, because we already support a surface at some point if we're doing a resize).

### Questions/Comments?

For figuring out major changes and iterative improvements to the library, check the tags. I try to document fairly thoroughly the changes I make, and try to stick to some kind of semantic versioning. Major version changes represent API breaking changes *and* likely changes to the structure or capabilities of the library on a large scale. Minor version changes should represent API breaking changes, but are not likely to be tied to major changes to the library. Hotfix changes are just as they sound: usually I find a bug I made that's just dreadful and needs to be fixed ASAP.

I'm always interested in improving my libraries and code! If you have any questions, comments, or criticism feel free to create an issue outlining them. If you're new to the Vulkan API and looking at designing your own Vulkan abstraction and want help, feel free to ask via an issue as well! We can take it to other, more private/thorough, contact methods from there
