# VulpesRender
#### Or, yet another Vulkan wrapper

This project has recently been re-scoped: instead of being a Vulkan renderer or 
framework, it's really a set of "primitives" that wrap Vulkan fuctionality and
objects into simple RAII classes. There's little "advanced" functionality here - 
the most advanced code is in the allocator subsystem, and much of the rest of the
project really is nothing but RAII wrappers over Vulkan objects.

Classes have been kept simple and to-the-point as much as possible, and have been
tuned to make using them easier. I tried to keep the "does what it says on the tin"
and "what you see if what you get" feeling of the raw Vulkan API mostly intact.
This project also features a minimal usage of the STL, insofar as I can manage. 
std::unique_ptr is used nearly everywhere to allow me to control initialization time and order, 
but still allowing me to keep RAII semantics.

There are a few big things on my to-do list, yet:
- [x] Fix the way debug callbacks and markers are handled (FIXED: via removing them altogether, for now)
- [ ] Consider finding a way to make this work as a dynamic library (WIP, GLFW doesn't like option toggles for dyn lib build)
- [ ] Make thread safety for objects storing vectors/arrays of objects more secure
- [ ] Implement the Pimpl idiom at some point (after dynamic library build is good to go)
- [ ] Improve the memory allocator, primarily trying to reduce fragmentation by dividing the memory pools/regions by size as well as type. This way there shouldn't be any more 256-512 byte buffers in between the much bigger vertex and index buffers.

Dependencies are handled in CMakeLists - GLFW will download and install
appropriately (including installing for projects that depend on this one). GLI is packaged in, as that has tests set on the global level by a CMake toggle and these tests break or act weirdly on newer versions of C++. If this is fixed, I'll add it back in as an ExternalProject in CMake or as a git submodule.
