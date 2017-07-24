Vulkan Rendering Framework.

- Currently only supports Windows. 
- Horrifyingly undocumented.
- Don't use this.

Known issues:

CMakeLists does not get the include directories correct, and doesn't include Vulkan's headers or its library. Same with glfw3. 
Librarian settings also incorrect: Should link library dependencies, add vulkan+glfw3dll+msvcrtd as additional dependencies. Ignore all default libraries.
