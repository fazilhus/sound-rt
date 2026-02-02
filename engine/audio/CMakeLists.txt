#--------------------------------------------------------------------------
# audio
#--------------------------------------------------------------------------

SET(files_audio
    listener.h
    listener.cc
    emitter.cc
    emitter.h
)
SOURCE_GROUP("audio" FILES ${files_audio})

SET(files_pch ../config.h ../config.cc)
SOURCE_GROUP("pch" FILES ${files_pch})
ADD_LIBRARY(audio STATIC ${files_audio} ${files_pch})
TARGET_PCH(audio ../)
ADD_DEPENDENCIES(audio soloud)
TARGET_LINK_LIBRARIES(audio PUBLIC engine soloud)