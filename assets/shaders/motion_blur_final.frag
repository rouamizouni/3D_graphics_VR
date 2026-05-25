#version 450 core

in vec2 TexCoords;

uniform sampler2D screenTexture;   // Scene colour (from FBO)
uniform sampler2D velocityTexture; // Per-pixel velocity (RG)
uniform int   numSamples;          // Quality: 8-24 samples recommended
uniform float blurIntensity;       // Scale multiplier on velocity
uniform float deltaTime;

out vec4 FragColor;

void main()
{
    // Sample velocity at this pixel
    vec2 vel = texture(velocityTexture, TexCoords).rg;

    // Scale velocity: real-time rendering uses camera exposure time as scale
    // RTR Chapter 12.5: blurVec = velocity * exposureTime * scale
    float exposureTime = deltaTime * 60.0; // normalise to 60 fps reference
    vec2 blurVec = vel * blurIntensity * exposureTime * 0.5;

    // Clamp maximum blur radius to avoid extreme smearing
    float maxBlur = 0.04;
    float blurLen = length(blurVec);
    if (blurLen > maxBlur)
        blurVec = normalize(blurVec) * maxBlur;

    // Accumulate samples along velocity direction
    vec4 result = vec4(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < numSamples; i++)
    {
        // t in [-0.5 .. 0.5] gives symmetric blur around current position
        float t = (float(i) / float(numSamples - 1)) - 0.5;
        vec2 offset = blurVec * t;
        vec2 sampleUV = TexCoords + offset;

        // Clamp to avoid sampling outside screen
        sampleUV = clamp(sampleUV, vec2(0.001), vec2(0.999));

        // Weight: gaussian falloff so centre sample weighs more
        float weight = exp(-6.0 * t * t);
        result      += texture(screenTexture, sampleUV) * weight;
        totalWeight += weight;
    }

    result /= totalWeight;

    // Preserve original if velocity is negligible (no blur needed)
    float velMagnitude = length(vel);
    float blendFactor  = smoothstep(0.0002, 0.005, velMagnitude);
    vec4 original      = texture(screenTexture, TexCoords);

    FragColor = mix(original, result, blendFactor);
}
