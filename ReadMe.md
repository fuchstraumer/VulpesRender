# VulpesRender
#### Or, yet another Vulkan wrapper

This project has recently been re-scoped: instead of being a Vulkan renderer or 
framework, it's really a set of "primitives" that wrap Vulkan fuctionality and
objects into simple RAII classes. There's little "advanced" functionality here - 
the most advanced code is in the allocator subsystem, and much of the rest of the
project really is nothing but RAII wrappers over Vulkan objects.

Classes have been kept simple and to-the-point as much as possible, and have been
tuned to make using them easier. This project also features a minimal usage of the
STL, insofar as I can manage. std::unique_ptr is used nearly everywhere to allow me 
to control initialization time and order, but still allowing me to keep RAII functionality.

There are a few big things on my to-do list, yet:
- [ ] Fix the way debug callbacks and markers are handled 
- [ ] Consider finding a way to make this work as a dynamic library
- [ ] Add some kind of input handling system: currently using ImGui for this (more on that)

Dependency wise, this project uses a few. ImGui is integrated as a submodule, so there's
no work required to get that setup. This is nearly only for the input handling done via ImGuiIO,
which is wonderfully handled and has made my life so much easier thus far. Eventually, however,
I want to replace this so I can remove this dependency. 

The other dependencies are handled in CMakeLists - GLFW, GLM, and GLI will all download and install
appropriately (including installing for projects that depend on this one)