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

### Usage

VPR can be used as a static or dynamic library (though the dynamic library usage could definitely use some improvement). The only
real dependency to speak of is GLFW, and this is all handled and linked in appropriately. 

I need to write more example documentation on how to use the various classes in this repository, but they're all
pretty simple and not far removed from how'd you see them used in official examples or things like Sascha's examples.

I welcome criticism, comments, and requests! I'm constantly trying to improve this project and my own skills, so all input 
is truly appreciated :)
