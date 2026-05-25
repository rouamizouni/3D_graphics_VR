#version 450 core

in vec2 texCoord;

uniform sampler2D screenTexture;
uniform float aberrationAmount = 0.005;  // Intensity of the effect
uniform float chaosAmount = 0.0;         // How chaotic the effect is (0-1)

out vec4 FragColor;

void main()
{
    // Chromatic aberration: shift RGB channels
    vec2 aberration = vec2(aberrationAmount * chaosAmount);
    
    // Sample each color channel with slight offsets
    float red = texture(screenTexture, texCoord + aberration).r;
    float green = texture(screenTexture, texCoord).g;
    float blue = texture(screenTexture, texCoord - aberration).b;
    
    // Combine the channels
    vec3 color = vec3(red, green, blue);
    
    // Add some edge detection glow for additional chaos effect
    if(chaosAmount > 0.5)
    {
        vec3 edge = vec3(0.0);
        vec2 offset = vec2(1.0 / 800.0, 1.0 / 600.0); // Adjust based on screen res
        
        // Simple Sobel-like edge detection
        float right = texture(screenTexture, texCoord + vec2(offset.x, 0)).r;
        float left = texture(screenTexture, texCoord - vec2(offset.x, 0)).r;
        float up = texture(screenTexture, texCoord + vec2(0, offset.y)).r;
        float down = texture(screenTexture, texCoord - vec2(0, offset.y)).r;
        
        float edgeMagnitude = abs(right - left) + abs(up - down);
        color += edgeMagnitude * chaosAmount * vec3(0.5, 0.0, 0.5);
    }
    
    FragColor = vec4(color, 1.0);
}
