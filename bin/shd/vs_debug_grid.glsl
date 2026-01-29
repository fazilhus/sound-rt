#version 430
layout(location = 0) in vec3 pos;
layout(location = 0) uniform mat4 ViewProjection;
void main()
{
	gl_Position = ViewProjection * vec4(pos, 1);
}