#version 450 core

in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoord;
    vec4 screenPos;
} fs_in;

uniform sampler2D texture_diffuse1;
uniform vec3 lightPos;
uniform vec3 viewPos;

// Light levels for cel shading
const vec3 levels[4] = vec3[](
    vec3(0.2),
    vec3(0.5),
    vec3(0.8),
    vec3(1.0)
);

out vec4 FragColor;

void main()
{
    vec3 color = texture(texture_diffuse1, fs_in.texCoord).rgb;
    
    // Normalize
    vec3 norm = normalize(fs_in.normal);
    vec3 lightDir = normalize(lightPos - fs_in.fragPos);
    
    // Calculate diffuse with cel shading levels
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Quantize to discrete levels
    float level = floor(diff * 4.0) / 4.0;
    vec3 diffuse = vec3(level) * color;
    
    // Add ambient
    vec3 ambient = vec3(0.3) * color;
    
    // Edge detection (Sobel-like)
    float edgeThreshold = 0.3;
    float edge = 1.0 - smoothstep(edgeThreshold, edgeThreshold + 0.1, diff);
    
    // Combine
    vec3 result = mix(diffuse + ambient, vec3(0.0), edge * 0.8);
    
    FragColor = vec4(result, 1.0);
}
