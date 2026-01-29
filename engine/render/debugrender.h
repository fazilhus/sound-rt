#pragma once

//------------------------------------------------------------------------------
/**
	@file	debugrender.h

	Contains debug rendering functions.

	@copyright
	(C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

namespace Physics {
    struct Plane;
    struct Ray;
    struct AABB;
    struct Simplex;
    struct ColliderId;
}

namespace Debug {
    enum RenderMode {
        Normal = 1,
        AlwaysOnTop = 2,
        WireFrame = 4
    };

    ///Draw text in screenspace
    void DrawDebugText(const char* text, glm::vec3 point, const glm::vec4 color);
    ///Draw line that spans from startpoint to endpoint (worldspace coordinates)
    void DrawLine(
        const glm::vec3& startPoint, const glm::vec3& endPoint, const float lineWidth, const glm::vec4& startColor,
        const glm::vec4& endColor, const RenderMode& renderModes = RenderMode::Normal
        );
    void DrawLine(
        const glm::vec3& startPoint, const glm::vec3& endPoint, const float lineWidth, const glm::vec4& color,
        const RenderMode& renderModes = RenderMode::Normal
        );
    void DrawTriangle(
        const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::mat4& t,
        const glm::vec4& color, float line_width = 1.0f, const RenderMode& render_mode = RenderMode::Normal
        );
    void DrawQuad(
        const glm::mat4& transform, const glm::vec4& color, const RenderMode render_mode = RenderMode::Normal,
        const float line_width = 1.0f
        );
    void DrawQuad(
        const glm::vec3& pos, const glm::quat& rot, const float scale, const glm::vec4& color,
        const RenderMode render_mode = RenderMode::Normal, const float line_width = 1.0f
        );
    void DrawQuad(
        const glm::vec3& pos, const glm::quat& rot, const glm::vec4& color,
        const float width, const float height,
        const RenderMode render_mode = RenderMode::Normal, const float line_width = 1.0f
        );
    ///Draws a unit colored cube at position with rotation and scale
    void DrawBox(
        const glm::vec3& position, const glm::quat& rotation, const float scale, const glm::vec4& color,
        const RenderMode renderModes = RenderMode::Normal, const float lineWidth = 1.0f
        );
    ///Draws a colored box at position with rotation. Size of box will be width (x-axis), height (y-axis), length (z-axis).
    void DrawBox(
        const glm::vec3& position, const glm::quat& rotation, const float width, const float height, const float length,
        const glm::vec4& color, const RenderMode renderModes = RenderMode::Normal, const float lineWidth = 1.0f
        );
    ///Draws a colored box with transform
    void DrawBox(
        const glm::mat4& transform, const glm::vec4& color, const RenderMode renderModes = RenderMode::Normal,
        const float lineWidth = 1.0f
        );

    void DrawGrid(
        const RenderMode renderModes = RenderMode::Normal, const float lineWidth = 1.0f
        );

    void DrawPlane(
        const Physics::Plane& plane, const RenderMode render_modes = RenderMode::Normal, const float line_width = 1.0f
        );
    void DrawRay(const Physics::Ray& ray, const glm::vec4& color, const float line_width = 1.0f);

    void DrawAABB(const Physics::AABB& aabb);
    void DrawAABBs();
    void DrawSelectedAABB();

    void DrawCMesh(Physics::ColliderId cm_id);
    void DrawCMeshes();
    void DrawSelectedCMesh();
    void DrawSimplex(const Physics::Simplex& s);

    void InitDebugRendering();
    void DispatchDebugDrawing();
    void DispatchDebugTextDrawing();
} // namespace Debug
