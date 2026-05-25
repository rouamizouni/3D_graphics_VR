#version 450 core

// Vertex attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Output to fragment shader
out VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoord;
    vec3 tangent;
    vec3 bitangent;
    vec4 prevPos;  // For motion blur
} vs_out;

void main()
{
    // Transform vertex to world space
    vs_out.fragPos = vec3(model * vec4(position, 1.0));
    
    // Transform normal to world space
    vs_out.normal = normalize(mat3(transpose(inverse(model))) * normal);
    
    // Pass texture coordinates
    vs_out.texCoord = texCoord;
    
    // Transform tangent and bitangent to world space
    vs_out.tangent = normalize(mat3(model) * tangent);
    vs_out.bitangent = normalize(mat3(model) * bitangent);
    
    // Store previous position for motion blur (will be updated by animation)
    vs_out.prevPos = projection * view * vec4(vs_out.fragPos, 1.0);
    
    // Final vertex position
    gl_Position = projection * view * vec4(vs_out.fragPos, 1.0);
}
