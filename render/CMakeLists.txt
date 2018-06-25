ADD_VPR_LIBRARY(vpr_render
    "include/Framebuffer.hpp"
    "include/GraphicsPipeline.hpp"
    "include/Renderpass.hpp"
    "src/Framebuffer.cpp"
    "src/GraphicsPipeline.cpp"
    "src/Renderpass.cpp"
)

TARGET_INCLUDE_DIRECTORIES(vpr_render PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
INSTALL(DIRECTORY "include/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/vpr/")
