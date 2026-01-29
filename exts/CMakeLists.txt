#--------------------------------------------------------------------------
# exts
#--------------------------------------------------------------------------

ADD_LIBRARY(exts INTERFACE)

ADD_SUBDIRECTORY(glew)
SET_TARGET_PROPERTIES(glew PROPERTIES FOLDER "exts/glew")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE glew/include)

SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
SET(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")		  

ADD_SUBDIRECTORY(glfw)
SET_TARGET_PROPERTIES(glfw PROPERTIES FOLDER "exts/glfw")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE glfw/include)

ADD_LIBRARY(imgui STATIC imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp 
			imgui/imgui.h imgui/imgui_internal.h imgui/imgui_tables.cpp imgui/imgui_widgets.cpp
			imgui/imstb_rectpack.h imgui/imstb_textedit.h imgui/imstb_truetype.h
			imgui/backends/imgui_impl_glfw.h imgui/backends/imgui_impl_glfw.cpp
            imgui/backends/imgui_impl_opengl3.h imgui/backends/imgui_impl_opengl3.cpp)

TARGET_LINK_LIBRARIES(imgui PUBLIC exts)
TARGET_INCLUDE_DIRECTORIES(imgui PUBLIC imgui imgui/backends)
SET_TARGET_PROPERTIES(imgui PROPERTIES FOLDER "exts/imgui")

TARGET_LINK_LIBRARIES(exts INTERFACE imgui)

ADD_SUBDIRECTORY(glm)
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE glm)

ADD_LIBRARY(stb_image STATIC stbi/include/stbi/stb_image.h stbi/include/stbi/stb_image_resize2.h stbi/include/stbi/stb_image_write.h stbi/stbi.c)
SET_TARGET_PROPERTIES(stb_image PROPERTIES FOLDER "exts/stb")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE stbi/include)
TARGET_LINK_LIBRARIES(exts INTERFACE stb_image)

ADD_LIBRARY(nlohmann_json STATIC nlohmann_json/include/nlohmann/json.hpp nlohmann_json/json.cpp)
SET_TARGET_PROPERTIES(nlohmann_json PROPERTIES FOLDER "exts/nlohmann_json")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE nlohmann_json/include)
TARGET_LINK_LIBRARIES(exts INTERFACE nlohmann_json)

ADD_LIBRARY(fx_gltf STATIC fx_gltf/include/fx/gltf.h fx_gltf/gltf.cpp)
TARGET_INCLUDE_DIRECTORIES(fx_gltf PRIVATE nlohmann_json/include)
SET_TARGET_PROPERTIES(fx_gltf PROPERTIES FOLDER "exts/fx_gltf")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE fx_gltf/include)
TARGET_LINK_LIBRARIES(fx_gltf INTERFACE nlohmann_json)
TARGET_LINK_LIBRARIES(exts INTERFACE fx_gltf)

if(WIN32)
    SET(SOLOUD_BACKEND_WINMM ON)
else()
    SET(SOLOUD_BACKEND_ALSA ON)
endif()
SET(SOLOUD_STATIC ON)

ADD_SUBDIRECTORY(soloud/contrib)
SET_TARGET_PROPERTIES(soloud PROPERTIES FOLDER "exts/soloud")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE soloud/include)