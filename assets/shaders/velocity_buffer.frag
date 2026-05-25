#version 450 core

in vec4 currClipPos;
in vec4 prevClipPos;

// RG16F texture — stores (velocityX, velocityY) in NDC space
layout(location = 0) out vec2 velocity;

void main()
{
    // Convert clip-space positions to NDC [-1..1]
    vec2 currNDC = currClipPos.xy / currClipPos.w;
    vec2 prevNDC = prevClipPos.xy / prevClipPos.w;

    // Velocity = displacement in NDC per frame
    velocity = currNDC - prevNDC;
}
