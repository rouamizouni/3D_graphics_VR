#version 410 core

// Full-screen quad shader drawn BEFORE anything else.
// Creates the black void with floating purple/red/pink light orbs.

in vec2 TexCoords;

uniform float time;
uniform vec2  resolution;
uniform int   piecesPlaced;   // 0-30, controls how much void fades out

out vec4 FragColor;

// Soft glowing circle
float orb(vec2 uv, vec2 centre, float radius, float softness)
{
    float d = length(uv - centre);
    return smoothstep(radius, radius - softness, d);
}

// Animated noise for subtle void shimmer
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i),           hash(i + vec2(1,0)), f.x),
        mix(hash(i + vec2(0,1)), hash(i + vec2(1,1)), f.x),
        f.y
    );
}

void main()
{
    vec2 uv = TexCoords;
    // Aspect-corrected UV centred at 0,0
    vec2 aUV = (uv - 0.5) * vec2(resolution.x / resolution.y, 1.0);

    // ── Base: pure black void ────────────────────────────────────
    vec3 col = vec3(0.0);

    // ── Subtle shimmer in the void ───────────────────────────────
    float shimmer = noise(aUV * 4.0 + time * 0.1) * 0.03;
    col += vec3(shimmer * 0.3, shimmer * 0.1, shimmer * 0.5);

    // ── Colored orbs — positions drift slowly with time ──────────
    // Purple orb (top-left area)
    vec2 purplePos = vec2(-0.5 + sin(time * 0.17) * 0.15,
                           0.25 + cos(time * 0.13) * 0.1);
    float purpleOrb = orb(aUV, purplePos, 0.18, 0.22);
    col += vec3(0.45, 0.0, 0.9) * purpleOrb * 0.6;

    // Pink orb (right area)
    vec2 pinkPos = vec2(0.52 + cos(time * 0.11) * 0.12,
                        -0.1 + sin(time * 0.19) * 0.15);
    float pinkOrb = orb(aUV, pinkPos, 0.14, 0.18);
    col += vec3(1.0, 0.2, 0.6) * pinkOrb * 0.55;

    // Deep red orb (bottom centre)
    vec2 redPos = vec2(sin(time * 0.09) * 0.2,
                       -0.35 + cos(time * 0.15) * 0.08);
    float redOrb = orb(aUV, redPos, 0.12, 0.16);
    col += vec3(0.9, 0.05, 0.15) * redOrb * 0.5;

    // Soft cyan accent (far left)
    vec2 cyanPos = vec2(-0.7 + sin(time * 0.07) * 0.05,
                         0.05 + cos(time * 0.21) * 0.12);
    float cyanOrb = orb(aUV, cyanPos, 0.09, 0.12);
    col += vec3(0.0, 0.7, 1.0) * cyanOrb * 0.4;

    // ── Radial glow halos around each orb ───────────────────────
    float halo1 = 0.04 / (length(aUV - purplePos) + 0.01);
    float halo2 = 0.03 / (length(aUV - pinkPos)   + 0.01);
    float halo3 = 0.025/ (length(aUV - redPos)     + 0.01);
    col += vec3(0.2, 0.0, 0.5) * clamp(halo1, 0.0, 0.3);
    col += vec3(0.5, 0.0, 0.3) * clamp(halo2, 0.0, 0.3);
    col += vec3(0.4, 0.0, 0.1) * clamp(halo3, 0.0, 0.3);

    // ── Fade void out as pieces are placed ───────────────────────
    // At 7+ pieces the skybox starts showing; void should fade
    float revealT = clamp(float(piecesPlaced - 4) / 7.0, 0.0, 1.0);
    col = mix(col, vec3(0.0), revealT * 0.8);

    // ── Vignette (darken edges) ──────────────────────────────────
    float vig = 1.0 - smoothstep(0.4, 1.0, length(aUV));
    col *= vig;

    // Tone-map (simple Reinhard so orbs don't blow out)
    col = col / (col + 0.5);

    FragColor = vec4(col, 1.0);
}
