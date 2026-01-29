#--------------------------------------------------------------------------
# physics
#--------------------------------------------------------------------------
SET(files_physics
    physicsresource.h
    phy.h
    phy.cc
    plane.h
    plane.cc
    ray.h
    physicsmesh.h
    physicsmesh.cc
    simplex.h
    simplex.cc
)
SOURCE_GROUP("physics" FILES ${files_physics})

SET(files_pch ../config.h ../config.cc)
SOURCE_GROUP("pch" FILES ${files_pch})
ADD_LIBRARY(physics STATIC ${files_physics} ${files_pch})
TARGET_PCH(physics ../)
ADD_DEPENDENCIES(physics glm)
TARGET_LINK_LIBRARIES(physics PUBLIC engine exts glm)