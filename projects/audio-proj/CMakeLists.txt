#--------------------------------------------------------------------------
# audio-proj project
#--------------------------------------------------------------------------

PROJECT(audio-proj)
FILE(GLOB project_headers code/*.h)
FILE(GLOB project_sources code/*.cc)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GSCEPT_LAB_ENV_OUTPUT_ROOT}/${PROJECT_NAME})

SET(files_project ${project_headers} ${project_sources})
SOURCE_GROUP("audio-proj" FILES ${files_project})

ADD_EXECUTABLE(audio-proj ${files_project})
TARGET_LINK_LIBRARIES(audio-proj core render audio)
ADD_DEPENDENCIES(audio-proj core render audio)

IF (MSVC)
    set_property(TARGET audio-proj PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
ENDIF ()