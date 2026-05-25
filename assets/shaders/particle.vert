#version 450 core
// particle.vert — billboarded sprite, expands a point to a camera-facing quad

layout(location = 0) in vec4 posSize;   // xyz=worldPos, w=size
layout(location = 1) in vec4 color;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;

out vec2 TexCoords;
out vec4 particleColor;

void main()
{
    // Billboard: expand point to camera-facing quad
    // This runs per-vertex of the GL_POINTS primitive
    // (driver expands each POINT to a quad for gl_PointSize)
    vec3 worldPos = posSize.xyz;
    float size    = posSize.w;

    gl_Position  = projection * view * vec4(worldPos, 1.0);
    gl_PointSize = size * 200.0 / gl_Position.w;  // screen-space size

    TexCoords     = vec2(0.5);  // placeholder — gl_PointCoord used in frag
    particleColor = color;
}
