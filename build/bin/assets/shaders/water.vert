#version 450 core

// Vertex attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float waveAmplitude;
uniform float waveFrequency;
uniform float waveSpeed;

// Output to fragment shader
out VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 texCoord;
    vec3 fragNormal;
    vec4 vertexColor;
    float depth;
} vs_out;

// Gerstner Wave calculation
vec3 gerstnerWave(vec4 wave, vec3 p) {
    float steepness = wave.z;
    float wavelength = wave.w;
    float k = 2.0 * 3.14159 / wavelength;
    float c = sqrt(9.8 / k);
    vec2 d = normalize(wave.xy);
    float f = k * (dot(d, p.xz) - c * time);
    float a = steepness / k;
    
    return vec3(
        d.x * (a * cos(f)),
        a * sin(f),
        d.y * (a * cos(f))
    );
}

// Calculate normal from waves
vec3 calculateWaveNormal(vec3 pos) {
    float epsilon = 0.1;
    
    vec3 p1 = gerstnerWave(vec4(1.0, 0.0, waveAmplitude, 50.0), pos);
    vec3 p2 = gerstnerWave(vec4(0.707, 0.707, waveAmplitude * 0.7, 31.0), pos);
    vec3 p3 = gerstnerWave(vec4(0.5, 0.866, waveAmplitude * 0.5, 18.0), pos);
    
    vec3 wave = p1 + p2 + p3;
    
    return normalize(normal + normalize(vec3(wave.x, 0.1, wave.z)));
}

void main()
{
    // Calculate wave displacement
    vec3 wavePos = position;
    
    vec3 wave1 = gerstnerWave(vec4(1.0, 0.0, waveAmplitude, 50.0), position);
    vec3 wave2 = gerstnerWave(vec4(0.707, 0.707, waveAmplitude * 0.7, 31.0), position);
    vec3 wave3 = gerstnerWave(vec4(0.5, 0.866, waveAmplitude * 0.5, 18.0), position);
    
    wavePos += wave1 + wave2 + wave3;
    
    // Transform position
    vs_out.fragPos = vec3(model * vec4(wavePos, 1.0));
    
    // Calculate normal
    vs_out.fragNormal = calculateWaveNormal(position);
    
    // Texture coordinates (animated)
    vs_out.texCoord = texCoord + vec2(time * 0.05, time * 0.03);
    
    // Depth for caustics effect
    vs_out.depth = length(vs_out.fragPos);
    
    // Vertex color based on height
    vs_out.vertexColor = vec4(
        0.2,
        0.4 + 0.3 * sin(wavePos.y * 5.0),
        0.8 + 0.2 * cos(wavePos.y * 3.0),
        0.7
    );
    
    // Final position
    gl_Position = projection * view * vec4(vs_out.fragPos, 1.0);
}
