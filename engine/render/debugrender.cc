//------------------------------------------------------------------------------
//  @file debugrender.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include <queue>
#include "debugrender.h"

#include <array>

#include "GL/glew.h"
#include "shaderresource.h"
#include "cameramanager.h"
#include "imgui.h"
#include "core/cvar.h"
#include "core/maths.h"
#include "physics/phy.h"
#include "physics/plane.h"
#include "physics/ray.h"
#include "physics/physicsmesh.h"
#include "physics/simplex.h"


namespace Debug {
    enum DebugShape {
        None,
        LINE,
        TRIANGLE,
        QUAD,
        SPHERE,
        BOX,
        CONE,
        CAPSULE,
        FRUSTUM,
        MESH,
        CIRCLE,
        GRID,
        NUM_DEBUG_SHAPES
    };

    struct RenderCommand {
        DebugShape shape{};
        char rendermode = Normal;
        float linewidth = 1.0f;
    };

    struct LineCommand : RenderCommand {
        glm::vec3 startpoint = glm::vec3(0.0f);
        glm::vec3 endpoint = glm::vec3(1.0f);
        glm::vec4 startcolor = glm::vec4(1.0f);
        glm::vec4 endcolor = glm::vec4(1.0f);
    };

    struct TriangleCommand : RenderCommand {
        glm::vec3 ps[3]{};
        glm::mat4 transform = glm::mat4();
        glm::vec4 color = glm::vec4(1.0f);
    };

    struct QuadCommand : RenderCommand {
        glm::mat4 transform = glm::mat4();
        glm::vec4 color = glm::vec4(1.0f);
    };

    struct BoxCommand : RenderCommand {
        glm::mat4 transform = glm::mat4();
        glm::vec4 color = glm::vec4(1.0f);
    };

    struct GridCommand : RenderCommand {};

    struct TextCommand {
        glm::vec4 point{};
        glm::vec4 color{};
        std::string text;
    };

    static std::queue<RenderCommand*> cmds;
    static std::queue<TextCommand> textcmds;
    static GLuint shaders[NUM_DEBUG_SHAPES];
    static GLuint vao[NUM_DEBUG_SHAPES];
    static GLuint ib[NUM_DEBUG_SHAPES];
    static GLuint vbo[NUM_DEBUG_SHAPES];

    void DrawDebugText(const char* text, glm::vec3 point, const glm::vec4 color) {
        TextCommand cmd;
        cmd.color = color;
        cmd.text = text;
        cmd.point = glm::vec4(point, 1.0f);
        textcmds.push(cmd);
    }

    void DrawLine(
        const glm::vec3& startPoint, const glm::vec3& endPoint, const float lineWidth, const glm::vec4& startColor,
        const glm::vec4& endColor, const RenderMode& renderModes
        ) {
        auto* cmd = new LineCommand();
        cmd->shape = LINE;
        cmd->startpoint = startPoint;
        cmd->endpoint = endPoint;
        cmd->linewidth = lineWidth;
        cmd->rendermode = renderModes;
        cmd->startcolor = startColor;
        cmd->endcolor = endColor;
        cmds.push(cmd);
    }

    void DrawLine(
        const glm::vec3& startPoint, const glm::vec3& endPoint, const float lineWidth, const glm::vec4& color,
        const RenderMode& renderModes
        ) {
        DrawLine(startPoint, endPoint, lineWidth, color, color, renderModes);
    }

    void DrawTriangle(
        const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::mat4& t,
        const glm::vec4& color, const float line_width, const RenderMode& render_mode
        ) {
        auto* cmd = new TriangleCommand();
        cmd->shape = TRIANGLE;
        cmd->linewidth = line_width;
        cmd->rendermode = render_mode;
        cmd->color = color;
        cmd->ps[0] = p1;
        cmd->ps[1] = p2;
        cmd->ps[2] = p3;
        cmd->transform = t;
        cmds.push(cmd);
    }

    void DrawQuad(
        const glm::mat4& transform, const glm::vec4& color, const RenderMode render_mode, const float line_width
        ) {
        auto* cmd = new QuadCommand();
        cmd->shape = QUAD;
        cmd->transform = transform;
        cmd->linewidth = line_width;
        cmd->color = color;
        cmd->rendermode = render_mode;
        cmds.push(cmd);
    }

    void DrawQuad(
        const glm::vec3& pos, const glm::quat& rot, const float scale, const glm::vec4& color,
        const RenderMode render_mode, const float line_width
        ) {
        const glm::mat4 t = glm::translate(pos) * static_cast<glm::mat4>(rot) * glm::scale(glm::vec3(scale));

        DrawQuad(t, color, render_mode, line_width);
    }

    void DrawQuad(
        const glm::vec3& pos, const glm::quat& rot, const glm::vec4& color, const float width,
        const float height, const RenderMode render_mode, const float line_width
        ) {
        const glm::mat4 t = glm::translate(pos) * static_cast<glm::mat4>(rot) * glm::scale(glm::vec3(width, height, 1.0f));

        DrawQuad(t, color, render_mode, line_width);
    }

    void DrawBox(
        const glm::vec3& position, const glm::quat& rotation, const float scale, const glm::vec4& color,
        const RenderMode renderModes, const float lineWidth
        ) {
        const glm::mat4 transform = glm::translate(position) * static_cast<glm::mat4>(rotation) * glm::scale(glm::vec3(scale));

        DrawBox(transform, color, renderModes, lineWidth);
    }

    void DrawBox(
        const glm::vec3& position, const glm::quat& rotation, const float width, const float height, const float length,
        const glm::vec4& color, const RenderMode renderModes, const float lineWidth
        ) {
        const glm::mat4 transform = glm::translate(position) * static_cast<glm::mat4>(rotation) * glm::scale(glm::vec3(width, height, length));

        DrawBox(transform, color, renderModes, lineWidth);
    }

    void DrawBox(
        const glm::mat4& transform, const glm::vec4& color, const RenderMode renderModes, const float lineWidth
        ) {
        auto* cmd = new BoxCommand();
        cmd->shape = BOX;
        cmd->transform = transform;
        cmd->linewidth = lineWidth;
        cmd->color = color;
        cmd->rendermode = renderModes;
        cmds.push(cmd);
    }

    void DrawGrid(const RenderMode renderModes, const float lineWidth) {
        auto* cmd = new GridCommand();
        cmd->shape = GRID;
        cmd->rendermode = renderModes;
        cmd->linewidth = lineWidth;
        cmds.push(cmd);
    }

    void DrawPlane(const Physics::Plane& plane, const RenderMode render_mode, const float line_width) {
        DrawQuad(plane.point(), Math::rot_vec3(plane.norm), 1.0f, glm::vec4(1,0,0,1), render_mode, line_width);
        DrawLine(plane.point(), plane.point() + plane.norm * 1.0f, line_width, glm::vec4(1,0,0,1), render_mode);
    }

    void DrawRay(const Physics::Ray& ray, const glm::vec4& color, const float line_width) {
        DrawLine(ray.orig, ray.orig + ray.dir, line_width, color, Normal);
    }

    void DrawAABB(const Physics::AABB& aabb) {
        DrawBox(
            0.5f * (aabb.max_bound + aabb.min_bound),
            glm::quat(),
            aabb.max_bound.x - aabb.min_bound.x,
            aabb.max_bound.y - aabb.min_bound.y,
            aabb.max_bound.z - aabb.min_bound.z,
            glm::vec4(0, 1, 0, 1),
            WireFrame,
            2.0f
            );
    }

    void DrawAABBs() {
        const auto& colliders = Physics::get_colliders();
        for (std::size_t i = 0; i < colliders.meshes.size(); ++i) {
            DrawAABB(colliders.aabbs[i]);
        }
    }

    static Core::CVar* r_draw_aabb = nullptr;
    static Core::CVar* r_draw_aabb_id = nullptr;
    static Core::CVar* r_draw_cm_id = nullptr;
    static Core::CVar* r_draw_cm_norm = nullptr;

    void DrawSelectedAABB() {
#if _DEBUG
        if (Core::CVarReadInt(r_draw_aabb) > 0) {
            const auto aabb_id = Core::CVarReadInt(r_draw_aabb_id);
            const auto& colliders = Physics::get_colliders();
            for (std::size_t i = 0; i < colliders.meshes.size(); ++i) {
                const auto& aabb = colliders.aabbs[i];
                if (aabb_id < 0 || i == aabb_id) {
                    DrawAABB(aabb);
                }
            }
        }
#endif
    }

    void DrawCMesh(const Physics::ColliderId cm_id) {
        const auto& colliders = Physics::get_colliders();
        const auto& cms = Physics::get_collider_meshes();
        const auto& mesh = cms.complex[colliders.meshes[cm_id.index].index];
        const auto& t = colliders.transforms[cm_id.index];
        const auto& s = colliders.states[cm_id.index];
        for (const auto& p : mesh.primitives) {
            for (const auto& tri : p.triangles) {
                Debug::DrawTriangle(
                    tri.v0, tri.v1, tri.v2,
                    t * glm::scale(glm::vec3(1.0f + 0.01f)),
                    glm::vec4(1,1,0,1),
                    1.0f,
                    (cm_id.index == Core::CVarReadInt(r_draw_cm_id) && tri.selected) ? Normal : WireFrame
                );
            }
        }
        Debug::DrawLine(
            s.dyn.pos - glm::vec3(s.dyn.angular_vel.x, s.dyn.angular_vel.y, s.dyn.angular_vel.z),
            s.dyn.pos + glm::vec3(s.dyn.angular_vel.x, s.dyn.angular_vel.y, s.dyn.angular_vel.z),
            2.0f,
            glm::vec4(0.5,0,1,1)
        );
    }

    void DrawCMeshes() {
        const auto& colliders = Physics::get_colliders();
        for (std::size_t cm_id = 0; cm_id < colliders.meshes.size(); ++cm_id) {
            DrawCMesh(Physics::ColliderId(cm_id));
        }
    }

    void DrawSelectedCMesh() {
#if _DEBUG
        const auto& colliders = Physics::get_colliders();
        if (const auto cm_id = Core::CVarReadInt(r_draw_cm_id);
            cm_id >= 0 && cm_id < colliders.meshes.size()) {
            DrawCMesh(Physics::ColliderId(cm_id));
        }
#endif
    }

    void DrawSimplex(const Physics::Simplex& s) {
        const auto a = s[0].point;
        const auto b = s[1].point;
        const auto c = s[2].point;
        const auto d = s[3].point;
        Debug::DrawTriangle(
            a, b, c, glm::mat4(1), glm::vec4(1, 0.5, 0.5, 1), 1,
            static_cast<RenderMode>(WireFrame | AlwaysOnTop)
            );
        Debug::DrawTriangle(
            a, d, b, glm::mat4(1), glm::vec4(1, 0.5, 0.5, 1), 1,
            static_cast<RenderMode>(WireFrame | AlwaysOnTop)
            );
        Debug::DrawTriangle(
            a, d, c, glm::mat4(1), glm::vec4(1, 0.5, 0.5, 1), 1,
            static_cast<RenderMode>(WireFrame | AlwaysOnTop)
            );
        Debug::DrawTriangle(
            b, c, d, glm::mat4(1), glm::vec4(1, 0.5, 0.5, 1), 1,
            static_cast<RenderMode>(WireFrame | AlwaysOnTop)
            );
    }

    void SetupShaders() {
        Render::ShaderResourceId const vsDebug = Render::ShaderResource::LoadShader(
            Render::ShaderResource::ShaderType::VERTEXSHADER,
            fs::create_path_from_rel_s("shd/vs_debug.glsl").c_str()
            );
        Render::ShaderResourceId const psDebug = Render::ShaderResource::LoadShader(
            Render::ShaderResource::ShaderType::FRAGMENTSHADER,
            fs::create_path_from_rel_s("shd/fs_debug.glsl").c_str()
            );
        Render::ShaderProgramId const progDebug = Render::ShaderResource::CompileShaderProgram({vsDebug, psDebug});
        shaders[TRIANGLE] = Render::ShaderResource::GetProgramHandle(progDebug);
        shaders[QUAD] = Render::ShaderResource::GetProgramHandle(progDebug);
        shaders[BOX] = Render::ShaderResource::GetProgramHandle(progDebug);

        Render::ShaderResourceId const vsLine = Render::ShaderResource::LoadShader(
            Render::ShaderResource::ShaderType::VERTEXSHADER,
            fs::create_path_from_rel_s("shd/vs_debug_lines.glsl").c_str()
            );
        Render::ShaderResourceId const psLine = Render::ShaderResource::LoadShader(
            Render::ShaderResource::ShaderType::FRAGMENTSHADER,
            fs::create_path_from_rel_s("shd/fs_debug_lines.glsl").c_str()
            );
        Render::ShaderProgramId const progLine = Render::ShaderResource::CompileShaderProgram({vsLine, psLine});
        shaders[LINE] = Render::ShaderResource::GetProgramHandle(progLine);

        Render::ShaderResourceId const vsGrid = Render::ShaderResource::LoadShader(
            Render::ShaderResource::ShaderType::VERTEXSHADER,
            fs::create_path_from_rel_s("shd/vs_debug_grid.glsl").c_str()
            );
        Render::ShaderResourceId const fsGrid = Render::ShaderResource::LoadShader(
            Render::ShaderResource::ShaderType::FRAGMENTSHADER,
            fs::create_path_from_rel_s("shd/fs_debug_grid.glsl").c_str()
            );
        Render::ShaderProgramId const progGrid = Render::ShaderResource::CompileShaderProgram({vsGrid, fsGrid});
        shaders[GRID] = Render::ShaderResource::GetProgramHandle(progGrid);
    }

    void SetupLine() {
        glGenVertexArrays(1, &vao[LINE]);
        glBindVertexArray(vao[LINE]);

        glGenBuffers(1, &vbo[LINE]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[LINE]);
        // buffer some dummy data. We use uniforms for setting the positions and colors of the lines.
        glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float), NULL, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat), NULL);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void SetupTriangle() {
        constexpr auto mesh_size = 9;
        constexpr GLfloat mesh[mesh_size] = {
            -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
            0.0f, 0.5f, 0.0f,
        };
        constexpr auto indices_size = 9;
        constexpr int indices[indices_size] = {
            0, 1, 2,

            0, 1,
            1, 2,
            2, 0,
        };

        glGenVertexArrays(1, &vao[TRIANGLE]);
        glBindVertexArray(vao[TRIANGLE]);

        glGenBuffers(1, &vbo[TRIANGLE]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[TRIANGLE]);
        glBufferData(GL_ARRAY_BUFFER, mesh_size * sizeof(GLfloat), mesh, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, NULL);

        glGenBuffers(1, &ib[TRIANGLE]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib[TRIANGLE]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size * sizeof(GLuint), indices, GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void SetupQuad() {
        constexpr auto mesh_size = 12;
        constexpr GLfloat mesh[mesh_size] = {
            -0.5f, 0.5f, 0.0f,
            0.5f, 0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
            0.5f, -0.5f, 0.0f,
        };
        constexpr auto indices_size = 18;
        constexpr int indices[indices_size] = {
            0, 1, 2,
            2, 1, 3,

            0, 1,
            1, 2,
            2, 0,
            1, 3,
            3, 2,
            0, 3
        };

        glGenVertexArrays(1, &vao[QUAD]);
        glBindVertexArray(vao[QUAD]);

        glGenBuffers(1, &vbo[QUAD]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[QUAD]);
        glBufferData(GL_ARRAY_BUFFER, mesh_size * sizeof(GLfloat), mesh, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, NULL);

        glGenBuffers(1, &ib[QUAD]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib[QUAD]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size * sizeof(GLuint), indices, GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void SetupBox() {
        constexpr auto meshSize = 24;
        constexpr GLfloat mesh[meshSize] =
        {
            0.5, -0.5, -0.5,
            0.5, -0.5, 0.5,
            -0.5, -0.5, 0.5,
            -0.5, -0.5, -0.5,
            0.5, 0.5, -0.5,
            0.5, 0.5, 0.5,
            -0.5, 0.5, 0.5,
            -0.5, 0.5, -0.5
        };
        constexpr auto indicesSize = 60;
        constexpr int indices[indicesSize] =
        {
            // triangles
            0, 1, 2,
            3, 0, 2,
            4, 7, 6,
            5, 4, 6,
            0, 4, 5,
            1, 0, 5,
            1, 5, 6,
            2, 1, 6,
            2, 6, 7,
            3, 2, 7,
            4, 0, 3,
            7, 4, 3,

            // outlines (offset 36)
            0, 1,
            1, 2,
            2, 3,
            3, 0,
            4, 5,
            5, 6,
            6, 7,
            7, 4,
            0, 4,
            1, 5,
            2, 6,
            3, 7
        };

        glGenVertexArrays(1, &vao[BOX]);
        glBindVertexArray(vao[BOX]);

        glGenBuffers(1, &vbo[BOX]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[BOX]);
        glBufferData(GL_ARRAY_BUFFER, meshSize * sizeof(GLfloat), mesh, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, NULL);

        glGenBuffers(1, &ib[BOX]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib[BOX]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize * sizeof(GLuint), indices, GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }


    namespace Internal {
        static constexpr size_t gridSize = 50;

        template <size_t GRIDSIZE>
        const std::array<float32, GRIDSIZE * 16> GenerateLineBuffer() {
            std::array<float32, GRIDSIZE * 16> arr = {};

            constexpr auto scale = 1.0f;
            const float32 max = (GRIDSIZE / 2); // top right
            const float32 min = -max; // top left

            for (size_t row = 0; row < GRIDSIZE; row++) {
                const size_t offset = row * 8;

                arr[offset] = (min) * scale;
                arr[offset + 1] = 0.0f;
                arr[offset + 2] = (min + row) * scale;
                arr[offset + 3] = 1.0f;

                arr[offset + 4] = (max) * scale;
                arr[offset + 5] = 0.0f;
                arr[offset + 6] = (min + row) * scale;
                arr[offset + 7] = 1.0f;
            }
            for (size_t col = 0; col < GRIDSIZE; col++) {
                const size_t offset = GRIDSIZE * 8 + col * 8;

                arr[offset] = (min + col) * scale;
                arr[offset + 1] = 0.0f;
                arr[offset + 2] = (min) * scale;
                arr[offset + 3] = 1.0f;

                arr[offset + 4] = (min + col) * scale;
                arr[offset + 5] = 0.0f;
                arr[offset + 6] = (max) * scale;
                arr[offset + 7] = 1.0f;
            }

            return arr;
        }
    } // namespace Internal

    void SetupGrid() {
        const auto buf = Internal::GenerateLineBuffer<Internal::gridSize>();
        glGenBuffers(1, &vbo[GRID]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[GRID]);
        glBufferData(GL_ARRAY_BUFFER, buf.size() * sizeof(float32), buf.data(), GL_STATIC_DRAW);

        glGenVertexArrays(1, &vao[GRID]);
        glBindVertexArray(vao[GRID]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float32) * 4, NULL);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    //------------------------------------------------------------------------------
    /**
	Setup debug rendering context
*/
    void InitDebugRendering() {
        SetupShaders();
        SetupLine();
        SetupTriangle();
        SetupQuad();
        SetupBox();
        SetupGrid();

        r_draw_aabb = Core::CVarCreate(Core::CVarType::CVar_Int, "r_draw_aabb", "0");
        r_draw_aabb_id = Core::CVarCreate(Core::CVarType::CVar_Int, "r_draw_aabb_id", "-1");
        r_draw_cm_id = Core::CVarCreate(Core::CVarType::CVar_Int, "r_draw_cm_id", "-1");
    }

    void RenderLine(RenderCommand* command) {
        auto* lineCommand = static_cast<LineCommand*>(command);

        glUseProgram(shaders[LINE]);

        if ((lineCommand->rendermode & AlwaysOnTop) == AlwaysOnTop) {
            glDepthFunc(GL_ALWAYS);
            glDepthRange(0.0f, 0.01f);
        }

        glPolygonMode(GL_FRONT, GL_LINE);
        glLineWidth(lineCommand->linewidth);

        glBindVertexArray(vao[LINE]);

        // This is so dumb, yet so much fun
        static GLint v0pos = glGetUniformLocation(shaders[LINE], "v0pos");
        static GLint v1pos = glGetUniformLocation(shaders[LINE], "v1pos");
        static GLint v0color = glGetUniformLocation(shaders[LINE], "v0color");
        static GLint v1color = glGetUniformLocation(shaders[LINE], "v1color");
        static GLint viewProjection = glGetUniformLocation(shaders[LINE], "viewProjection");

        // Upload uniforms for positions and colors
        glUniform4fv(v0pos, 1, &lineCommand->startpoint[0]);
        glUniform4fv(v1pos, 1, &lineCommand->endpoint[0]);
        glUniform4fv(v0color, 1, &lineCommand->startcolor[0]);
        glUniform4fv(v1color, 1, &lineCommand->endcolor[0]);

        const Render::Camera* const mainCamera = Render::CameraManager::GetCamera(CAMERA_MAIN);
        glUniformMatrix4fv(viewProjection, 1, GL_FALSE, &mainCamera->viewProjection[0][0]);

        glDrawArrays(GL_LINES, 0, 2);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glPolygonMode(GL_FRONT, GL_FILL);

        if ((lineCommand->rendermode & AlwaysOnTop) == AlwaysOnTop) {
            glDepthFunc(GL_LEQUAL);
            glDepthRange(0.0f, 1.0f);
        }
    }

    void RenderTriangle(RenderCommand* command) {
        const auto cmd = static_cast<TriangleCommand*>(command);

        constexpr auto mesh_size = 9;
        const GLfloat mesh[mesh_size] = {
            cmd->ps[0].x, cmd->ps[0].y, cmd->ps[0].z,
            cmd->ps[1].x, cmd->ps[1].y, cmd->ps[1].z,
            cmd->ps[2].x, cmd->ps[2].y, cmd->ps[2].z,
        };

        glUseProgram(shaders[TRIANGLE]);
        glBindVertexArray(vao[TRIANGLE]);

        glBindBuffer(GL_ARRAY_BUFFER, vbo[TRIANGLE]);
        glBufferData(GL_ARRAY_BUFFER, mesh_size * sizeof(GLfloat), mesh, GL_STATIC_DRAW);

        const GLint loc = glGetUniformLocation(shaders[TRIANGLE], "color");
        glUniform4fv(loc, 1, &cmd->color.x);

        static GLint model = glGetUniformLocation(shaders[TRIANGLE], "model");
        static GLint viewProjection = glGetUniformLocation(shaders[TRIANGLE], "viewProjection");
        const Render::Camera* const mainCamera = Render::CameraManager::GetCamera(CAMERA_MAIN);
        glUniformMatrix4fv(model, 1, GL_FALSE, &cmd->transform[0][0]);
        glUniformMatrix4fv(viewProjection, 1, GL_FALSE, &mainCamera->viewProjection[0][0]);

        glDisable(GL_CULL_FACE);
        if ((cmd->rendermode & AlwaysOnTop) == AlwaysOnTop) {
            glDepthFunc(GL_ALWAYS);
            glDepthRange(0.0f, 0.01f);
        }

        if ((cmd->rendermode & WireFrame) == WireFrame) {
            glPolygonMode(GL_FRONT, GL_LINE);
            glLineWidth(cmd->linewidth);

            glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, reinterpret_cast<void*>(3 * sizeof(GLuint)));
            glPolygonMode(GL_FRONT, GL_FILL);
        }
        else { glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, NULL); }

        if ((cmd->rendermode & AlwaysOnTop) == AlwaysOnTop) {
            glDepthFunc(GL_LEQUAL);
            glDepthRange(0.0f, 1.0f);
        }
        glEnable(GL_CULL_FACE);

        glBindVertexArray(0);
    }

    void RenderQuad(RenderCommand* command) {
        const auto cmd = static_cast<QuadCommand*>(command);

        glUseProgram(shaders[QUAD]);
        glBindVertexArray(vao[QUAD]);

        const GLint loc = glGetUniformLocation(shaders[QUAD], "color");
        glUniform4fv(loc, 1, &cmd->color.x);

        static GLint model = glGetUniformLocation(shaders[QUAD], "model");
        static GLint viewProjection = glGetUniformLocation(shaders[QUAD], "viewProjection");
        const Render::Camera* const mainCamera = Render::CameraManager::GetCamera(CAMERA_MAIN);
        glUniformMatrix4fv(model, 1, GL_FALSE, &cmd->transform[0][0]);
        glUniformMatrix4fv(viewProjection, 1, GL_FALSE, &mainCamera->viewProjection[0][0]);

        glDisable(GL_CULL_FACE);
        if ((cmd->rendermode & AlwaysOnTop) == AlwaysOnTop) {
            glDepthFunc(GL_ALWAYS);
            glDepthRange(0.0f, 0.01f);
        }

        if ((cmd->rendermode & WireFrame) == WireFrame) {
            glPolygonMode(GL_FRONT, GL_LINE);
            glLineWidth(cmd->linewidth);

            glDrawElements(GL_LINES, 12, GL_UNSIGNED_INT, reinterpret_cast<void*>(6 * sizeof(GLuint)));
            glPolygonMode(GL_FRONT, GL_FILL);
        }
        else { glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL); }

        if ((cmd->rendermode & AlwaysOnTop) == AlwaysOnTop) {
            glDepthFunc(GL_LEQUAL);
            glDepthRange(0.0f, 1.0f);
        }
        glEnable(GL_CULL_FACE);

        glBindVertexArray(0);
    }

    void RenderBox(RenderCommand* command) {
        const auto* cmd = static_cast<BoxCommand*>(command);

        glUseProgram(shaders[BOX]);

        glBindVertexArray(vao[BOX]);

        static GLint loc = glGetUniformLocation(shaders[BOX], "color");
        glUniform4fv(loc, 1, &cmd->color.x);

        static GLint model = glGetUniformLocation(shaders[BOX], "model");
        static GLint viewProjection = glGetUniformLocation(shaders[BOX], "viewProjection");
        const Render::Camera* const mainCamera = Render::CameraManager::GetCamera(CAMERA_MAIN);
        glUniformMatrix4fv(model, 1, GL_FALSE, &cmd->transform[0][0]);
        glUniformMatrix4fv(viewProjection, 1, GL_FALSE, &mainCamera->viewProjection[0][0]);

        if ((cmd->rendermode & AlwaysOnTop) == AlwaysOnTop) {
            glDepthFunc(GL_ALWAYS);
            glDepthRange(0.0f, 0.01f);
        }

        if ((cmd->rendermode & WireFrame) == WireFrame) {
            glPolygonMode(GL_FRONT, GL_LINE);
            glLineWidth(cmd->linewidth);

            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, reinterpret_cast<void*>(36 * sizeof(GLuint)));
            glPolygonMode(GL_FRONT, GL_FILL);
        }
        else { glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL); }

        if ((cmd->rendermode & AlwaysOnTop) == AlwaysOnTop) {
            glDepthFunc(GL_LEQUAL);
            glDepthRange(0.0f, 1.0f);
        }

        glBindVertexArray(0);
    }

    void RenderGrid(RenderCommand*) {
        glUseProgram(shaders[GRID]);
        glBindVertexArray(vao[GRID]);
        const Render::Camera* const mainCamera = Render::CameraManager::GetCamera(CAMERA_MAIN);
        glUniformMatrix4fv(0, 1, false, &mainCamera->viewProjection[0][0]);
        glDrawArrays(GL_LINES, 0, Internal::gridSize * 2 * 2);
        glBindVertexArray(0);
    }

    void DispatchDebugDrawing() {
        while (!cmds.empty()) {
            RenderCommand* currentCommand = cmds.front();
            cmds.pop();
            switch (currentCommand->shape) {
            case LINE: {
                RenderLine(currentCommand);
                break;
            }
            case TRIANGLE: {
                RenderTriangle(currentCommand);
                break;
            }
            case QUAD: {
                RenderQuad(currentCommand);
                break;
            }
            case BOX: {
                RenderBox(currentCommand);
                break;
            }
            case GRID: {
                RenderGrid(currentCommand);
                break;
            }
            default: {
                n_assert2(false, "Debug::RenderShape not fully implemented!\n");
                break;
            }
            } // switch

            delete currentCommand;
        }
    }

    void DispatchDebugTextDrawing() {
        if (textcmds.empty())
            return;

        static bool open = true;
        ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize, ImGuiCond_Always);
        ImGui::Begin(
            "TEXT_RENDERING", &open,
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoNav
            );

        Render::Camera* const cam = Render::CameraManager::GetCamera(CAMERA_MAIN);

        while (!textcmds.empty()) {
            TextCommand& cmd = textcmds.front();

            // transform point into screenspace
            cmd.point.w = 1.0f;
            glm::vec4 ndc = cam->viewProjection * cmd.point;
            ndc /= ndc.w;

            if (ndc.z <= 1) // only render in front of camera
            {
                glm::vec2 cursorPos = {ndc.x, ndc.y};
                cursorPos += glm::vec2(1.0f, 1.0f);
                cursorPos *= 0.5f;
                cursorPos.y = 1.0f - cursorPos.y;
                cursorPos.x *= ImGui::GetWindowWidth();
                cursorPos.y *= ImGui::GetWindowHeight();
                // center text
                cursorPos.x -= ImGui::CalcTextSize(cmd.text.c_str()).x / 2.0f;

                ImGui::SetCursorPos({cursorPos.x, cursorPos.y});

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(cmd.color.x, cmd.color.y, cmd.color.z, cmd.color.w));
                ImGui::TextUnformatted(cmd.text.c_str());
                ImGui::PopStyleColor();
            }
            textcmds.pop();
        }
        ImGui::End();
    }
} // namespace Debug
