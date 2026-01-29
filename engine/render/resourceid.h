#pragma once

namespace Render {
    using ResourceId = unsigned int;
    constexpr ResourceId InvalidResourceId = UINT_MAX;

    using ShaderResourceId = ResourceId;
    using ShaderProgramId = ResourceId;

    using ModelId = ResourceId;
    using BrushGroupId = ResourceId;

    using TextureResourceId = ResourceId;
}