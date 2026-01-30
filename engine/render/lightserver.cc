//------------------------------------------------------------------------------
//  @file lightserver.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "GL/glew.h"
#include "lightserver.h"
#include "model.h"
#include "cameramanager.h"
#include "core/cvar.h"
#include "core/idpool.h"
#include "core/filesystem.h"


namespace Render::LightServer {
    struct VisibleIndex {
        int index;
    };

    enum LightType {
        NaN = 1,
        Point = 2,
        Spot = 4,
        Directional = 8
    };

    struct PointLights {
        std::vector<glm::vec4> positions;
        std::vector<glm::vec4> colors;
        std::vector<float> radii;
        GLuint buffers[4]{};
    };

    glm::vec3 globalLightDirection;
    glm::vec3 globalLightColor;

    static ModelId icoSphereModel;
    static Util::IdPool<PointLightId> pointLightPool;

    static PointLights pointLights;

    constexpr GLuint maxTileLights = 512;
    constexpr GLuint maxTileProbes = 128;
    static GLuint workGroupsX = 0;
    static GLuint workGroupsY = 0;

    static GLuint globalShadowMap;
    static GLuint globalShadowFrameBuffer;
    constexpr unsigned int shadowMapSize = 4096;

    static Core::CVar* r_draw_light_spheres = nullptr;
    static Core::CVar* r_draw_light_sphere_id = nullptr;

    //------------------------------------------------------------------------------
    /**
*/
    void Initialize() {
        globalLightDirection = glm::normalize(glm::vec3(-0.1f, -0.77735f, -0.27735f));
        globalLightColor = glm::vec3(1.0f, 0.8f, 0.8f) * 3.0f;

        icoSphereModel = LoadModel(fs::create_path_from_rel_s("assets/system/icosphere.glb"));

        r_draw_light_spheres = Core::CVarCreate(Core::CVarType::CVar_Int, "r_draw_light_spheres", "0");
        r_draw_light_sphere_id = Core::CVarCreate(Core::CVarType::CVar_Int, "r_draw_light_sphere_id", "-1");

        glGenBuffers(static_cast<GLuint>(PointLightBuffer::NUM_BUFFERS), pointLights.buffers);

        // setup shadow pass
        glGenTextures(1, &globalShadowMap);
        glBindTexture(GL_TEXTURE_2D, globalShadowMap);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr
            );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glGenFramebuffers(1, &globalShadowFrameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, globalShadowFrameBuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, globalShadowMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        // setup a shadow camera
        CameraCreateInfo shadowCameraInfo;
        shadowCameraInfo.hash = CAMERA_SHADOW;
        shadowCameraInfo.projection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, 0.1f, 500.0f);
        shadowCameraInfo.view = glm::lookAt(
            glm::vec3(-10.0f, 75.0f, -20.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
            );
        globalLightDirection = shadowCameraInfo.view[2];
        CameraManager::CreateCamera(shadowCameraInfo);
    }

    //------------------------------------------------------------------------------
    /**
*/
    void UpdateWorkGroups(const uint resolutionWidth, const uint resolutionHeight) {
        // Define work group sizes in x and y direction based off screen size and tile size (in pixels)
        constexpr auto TILE_SIZE = 32;
        workGroupsX = (resolutionWidth + (resolutionWidth % TILE_SIZE)) / TILE_SIZE;
        workGroupsY = (resolutionHeight + (resolutionHeight % TILE_SIZE)) / TILE_SIZE;

        const size_t numberOfTiles = workGroupsX * workGroupsY;

        // Bind visible Point light indices buffer
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointLights.buffers[static_cast<GLuint>(PointLightBuffer::VISIBLE_INDICES)]);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER, numberOfTiles * sizeof(VisibleIndex) * maxTileLights, nullptr, GL_DYNAMIC_DRAW
            );
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    //------------------------------------------------------------------------------
    /**
*/
    void OnBeforeRender() {
        static size_t prevNumPointLights = 0;

        if (const size_t numPointLights = pointLights.positions.size();
            prevNumPointLights != numPointLights) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointLights.buffers[static_cast<GLuint>(PointLightBuffer::POSITIONS)]);
            glBufferData(
                GL_SHADER_STORAGE_BUFFER, numPointLights * sizeof(glm::vec4), pointLights.positions.data(),
                GL_DYNAMIC_DRAW
                );
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointLights.buffers[static_cast<GLuint>(PointLightBuffer::COLORS)]);
            glBufferData(
                GL_SHADER_STORAGE_BUFFER, numPointLights * sizeof(glm::vec4), pointLights.colors.data(),
                GL_DYNAMIC_DRAW
                );
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointLights.buffers[static_cast<GLuint>(PointLightBuffer::RADII)]);
            glBufferData(
                GL_SHADER_STORAGE_BUFFER, numPointLights * sizeof(float), pointLights.radii.data(), GL_DYNAMIC_DRAW
                );
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            prevNumPointLights = numPointLights;
        }
    }

    //------------------------------------------------------------------------------
    /**
*/
    void Update(const ShaderProgramId pid) {
        const GLuint programHandle = ShaderResource::GetProgramHandle(pid);
        glUniform3fv(glGetUniformLocation(programHandle, "GlobalLightDirection"), 1, &globalLightDirection[0]);
        glUniform3fv(glGetUniformLocation(programHandle, "GlobalLightColor"), 1, &globalLightColor[0]);
    }

    //------------------------------------------------------------------------------
    /**
*/
    void DebugDrawPointLights() {
#if _DEBUG
        if (Core::CVarReadInt(r_draw_light_spheres) > 0) {
            Model::Mesh::Primitive const& primitive = GetModel(icoSphereModel).meshes[0].primitives[0];
            glBindVertexArray(primitive.vao);

            static ShaderResourceId const vs = ShaderResource::LoadShader(
                ShaderResource::ShaderType::VERTEXSHADER,
                fs::create_path_from_rel_s("shd/vs_debug.glsl").c_str()
                );
            static ShaderResourceId const fs = ShaderResource::LoadShader(
                ShaderResource::ShaderType::FRAGMENTSHADER,
                fs::create_path_from_rel_s("shd/fs_debug.glsl").c_str()
                );
            static ShaderProgramId debugProgram = ShaderResource::CompileShaderProgram({vs, fs});
            const GLuint debugProgramHandle = ShaderResource::GetProgramHandle(debugProgram);
            glUseProgram(debugProgramHandle);

            glm::vec4 color(1, 0, 0, 1);
            glUniform4fv(glGetUniformLocation(debugProgramHandle, "color"), 1, &color.x);

            static GLint model = glGetUniformLocation(debugProgramHandle, "model");
            static GLint viewProjection = glGetUniformLocation(debugProgramHandle, "viewProjection");
            const Camera* const mainCamera = CameraManager::GetCamera(CAMERA_MAIN);
            glUniformMatrix4fv(viewProjection, 1, GL_FALSE, &mainCamera->viewProjection[0][0]);

            glDisable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            const int drawId = Core::CVarReadInt(r_draw_light_sphere_id);
            for (auto i = 0; i < pointLights.positions.size(); i++) {
                if (drawId < 0 || i == drawId) {
                    glm::mat4 transform = glm::translate(glm::vec3(pointLights.positions[i])) * glm::scale(
                        glm::vec3(pointLights.radii[i])
                        );
                    glUniformMatrix4fv(model, 1, GL_FALSE, &transform[0][0]);
                    glDrawElements(
                        GL_TRIANGLES, primitive.numIndices, primitive.indexType, reinterpret_cast<void*>(static_cast<intptr_t>(primitive.offset))
                        );
                }
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_CULL_FACE);

            glBindVertexArray(0);
        }
#endif
    }

    //------------------------------------------------------------------------------
    /**
*/
    bool IsValid(const PointLightId id) { return pointLightPool.IsValid(id); }

    //------------------------------------------------------------------------------
    /**
*/
    PointLightId CreatePointLight(const glm::vec3& position, const glm::vec3& color, const float intensity, const float radius) {
        PointLightId id{};
        if (pointLightPool.Allocate(id)) {
            pointLights.positions.emplace_back(position, 1);
            pointLights.colors.emplace_back(glm::vec4(color, 1) * intensity);
            pointLights.radii.emplace_back(radius);
        }
        else {
            pointLights.positions[id.index] = glm::vec4(position, 1);
            pointLights.colors[id.index] = glm::vec4(color, 1) * intensity;
            pointLights.radii[id.index] = radius;
        }
        return id;
    }

    //------------------------------------------------------------------------------
    /**
*/
    void DestroyPointLight(const PointLightId id) {
        assert(false);
        // TODO: Need to add a hashtable to be able to remove lights, and still keep them packed.
        pointLightPool.Deallocate(id);
    }

    //------------------------------------------------------------------------------
    /**
*/
    void SetPosition(const PointLightId id, const glm::vec3& position) {
        assert(IsValid(id));
        pointLights.positions[id.index] = glm::vec4(position, 1);
    }

    //------------------------------------------------------------------------------
    /**
*/
    glm::vec3 GetPosition(const PointLightId id) {
        assert(IsValid(id));
        return pointLights.positions[id.index];
    }

    //------------------------------------------------------------------------------
    /**
*/
    void SetColorAndIntensity(const PointLightId id, const glm::vec3& color, const float intensity) {
        assert(IsValid(id));
        pointLights.colors[id.index] = glm::vec4(color, 1) * intensity;
    }

    //------------------------------------------------------------------------------
    /**
*/
    glm::vec3 GetColorAndIntensity(const PointLightId id) {
        assert(IsValid(id));
        return pointLights.colors[id.index];
    }

    //------------------------------------------------------------------------------
    /**
*/
    void SetRadius(const PointLightId id, const float radius) {
        assert(IsValid(id));
        pointLights.radii[id.index] = radius;
    }

    //------------------------------------------------------------------------------
    /**
*/
    float GetRadius(const PointLightId id) {
        assert(IsValid(id));
        return pointLights.radii[id.index];
    }

    //------------------------------------------------------------------------------
    /**
*/
    GLuint GetBuffer(PointLightBuffer buf) {
        assert(static_cast<int>(buf) < static_cast<int>(PointLightBuffer::NUM_BUFFERS));
        return pointLights.buffers[static_cast<GLuint>(buf)];
    }

    //------------------------------------------------------------------------------
    /**
*/
    GLuint GetWorkGroupsX() { return workGroupsX; }

    //------------------------------------------------------------------------------
    /**
*/
    GLuint GetWorkGroupsY() { return workGroupsY; }

    //------------------------------------------------------------------------------
    /**
*/
    size_t GetNumPointLights() { return pointLights.positions.size(); }

    //------------------------------------------------------------------------------
    /**
*/
    GLuint GetGlobalShadowMapHandle() { return globalShadowMap; }

    //------------------------------------------------------------------------------
    /**
*/
    GLuint GetGlobalShadowFramebuffer() { return globalShadowFrameBuffer; }

    //------------------------------------------------------------------------------
    /**
*/
    uint GetShadowMapSize() { return shadowMapSize; }
}
