#version 450 core

layout(location = 0) in vec3 position;

uniform mat4 view;
uniform mat4 projection;

out vec3 texCoords;

void main()
{
    texCoords = position;
    // Strip translation from view matrix so skybox stays fixed
    mat4 viewNoTranslation = mat4(mat3(view));
    vec4 pos = projection * viewNoTranslation * vec4(position, 1.0);
    // Set z = w so skybox always renders at far plane
    gl_Position = pos.xyww;
}
