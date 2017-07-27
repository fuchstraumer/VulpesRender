Vulkan Rendering Framework.

- Currently only supports Windows. 
- Horrifyingly undocumented.
- Don't use this.
- If you do, pls help me make it better

Known issues:

CMakeLists does not get the include directories correct, and doesn't include Vulkan's headers or its library. Same with glfw3. 
Librarian settings also incorrect: Should link library dependencies. Common runtime components no longer required. glfw3 is linked statically, currently, and this library can only be used as a static library.
