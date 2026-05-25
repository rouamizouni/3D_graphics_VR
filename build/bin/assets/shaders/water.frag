#version 450 core

in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoord;
    vec3 fragNormal;
    vec4 vertexColor;
    float depth;
} fs_in;

uniform sampler2D waterTexture;
uniform sampler2D normalMap;
uniform sampler2D foamTexture;
uniform vec3 viewPos;
uniform float time;

layout(location = 0) out vec4 FragColor;

void main()
{
    // Sample textures
    vec3 texColor = texture(waterTexture, fs_in.texCoord).rgb;
    vec3 normalMapSample = texture(normalMap, fs_in.texCoord * 2.0).rgb;
    
    // Blend normals
    vec3 blendedNormal = normalize(fs_in.fragNormal + (normalMapSample - 0.5) * 0.3);
    
    // View direction
    vec3 viewDir = normalize(viewPos - fs_in.fragPos);
    
    // Fresnel effect (more transparent at shallow angles)
    float fresnel = pow(1.0 - abs(dot(viewDir, blendedNormal)), 3.0);
    fresnel = mix(0.2, 1.0, fresnel);
    
    // Specular highlight (sun reflection)
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 halfDir = normalize(lightDir + viewDir);
    float specular = pow(max(dot(blendedNormal, halfDir), 0.0), 32.0);
    
    // Caustics (underwater light pattern)
    float caustics = sin(fs_in.fragPos.x * 2.0 + time) * cos(fs_in.fragPos.z * 2.0 - time) * 0.5 + 0.5;
    caustics *= sin(fs_in.fragPos.x * 0.5 + time * 0.5) * 0.3 + 0.5;
    
    // Foam based on height
    float foam = fs_in.vertexColor.g - 0.35;
    foam = max(0.0, foam) * 2.0;
    foam *= texture(foamTexture, fs_in.texCoord * 4.0 + time * 0.1).r;
    
    // Depth-based color
    float depthFade = exp(-fs_in.depth / 50.0);
    
    // Water color with depth
    vec3 waterColor = mix(
        vec3(0.1, 0.2, 0.3),  // Deep water
        vec3(0.3, 0.6, 0.7),  // Shallow water
        depthFade
    );
    
    // Combine effects
    vec3 finalColor = waterColor;
    finalColor += texColor * 0.3;
    finalColor += specular * vec3(1.0, 0.95, 0.8) * 0.5;
    finalColor += caustics * vec3(0.3, 0.3, 0.2) * 0.4;
    finalColor += foam * vec3(1.0, 1.0, 1.0);
    
    // Apply fresnel and vertex color
    finalColor = mix(finalColor, fs_in.vertexColor.rgb, 0.4);
    
    // Alpha for transparency
    float alpha = mix(0.6, 0.9, fresnel);
    
    FragColor = vec4(finalColor, alpha);
}
