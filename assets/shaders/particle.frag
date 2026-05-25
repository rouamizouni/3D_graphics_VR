#version 450 core

in vec4 particleColor;

uniform sampler2D particleTexture;

out vec4 FragColor;

void main()
{
    // gl_PointCoord gives [0..1] across the point sprite
    vec2 uv   = gl_PointCoord;
    vec4 tex  = texture(particleTexture, uv);

    // Soft circle falloff even without a texture
    float dist   = length(uv - vec2(0.5));
    float circle = smoothstep(0.5, 0.3, dist);

    vec4 col = particleColor * tex;
    col.a   *= circle;

    if (col.a < 0.01) discard;
    FragColor = col;
}
