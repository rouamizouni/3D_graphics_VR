#version 450 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoord;
    vec4 screenPos;
} vs_out;

void main()
{
    vs_out.fragPos = vec3(model * vec4(position, 1.0));
    vs_out.normal = normalize(mat3(transpose(inverse(model))) * normal);
    vs_out.texCoord = texCoord;
    
    vec4 clipPos = projection * view * vec4(vs_out.fragPos, 1.0);
    vs_out.screenPos = clipPos;
    gl_Position = clipPos;
}
