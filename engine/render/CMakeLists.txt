#--------------------------------------------------------------------------
# render
#--------------------------------------------------------------------------

SET(files_render_display
    window.h
    window.cc
)
SOURCE_GROUP("display" FILES ${files_render_display})

SET(files_render_render
    model.h
    model.cc
    renderdevice.h
    renderdevice.cc
    shaderresource.h
    shaderresource.cc
    textureresource.h
    textureresource.cc
    lightserver.h
    lightserver.cc
    cameramanager.h
    cameramanager.cc
    debug_camera.h
    debug_camera.cc
    debugrender.h
    debugrender.cc
    lightsources.h
    resourceid.h
    particlesystem.cc
    particlesystem.h
)
SOURCE_GROUP("render" FILES ${files_render_render})

SET(files_render_input
    input/gamepad.h
    input/gamepad.cc
    input/inputserver.h
    input/inputserver.cc
    input/key.h
    input/key.cc
    input/keyboard.h
    input/mouse.h
)
SOURCE_GROUP("input" FILES ${files_render_input})

SET(files_render
    ${files_render_display}
    ${files_render_render}
    ${files_render_pch}
    ${files_render_input})

SET(files_pch ../config.h ../config.cc)
SOURCE_GROUP("pch" FILES ${files_pch})
ADD_LIBRARY(render STATIC ${files_render} ${files_pch})
TARGET_PCH(render ../)
ADD_DEPENDENCIES(render exts imgui glew glfw glm::glm)
TARGET_LINK_LIBRARIES(render PUBLIC engine exts glew glfw imgui ${OPENGL_LIBS} glm::glm)