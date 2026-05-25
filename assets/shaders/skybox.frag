#version 450 core

in vec3 texCoords;

uniform samplerCube skybox;
uniform float timeOfDay;   // 0.0 = dawn, 0.5 = noon, 1.0 = dusk

out vec4 FragColor;

void main()
{
    vec4 skyColor = texture(skybox, texCoords);

    // Time-of-day tinting
    vec3 dawnTint  = vec3(1.0, 0.6, 0.3);
    vec3 noonTint  = vec3(1.0, 1.0, 1.0);
    vec3 duskTint  = vec3(1.0, 0.4, 0.2);

    vec3 tint;
    if (timeOfDay < 0.5)
        tint = mix(dawnTint, noonTint, timeOfDay * 2.0);
    else
        tint = mix(noonTint, duskTint, (timeOfDay - 0.5) * 2.0);

    FragColor = vec4(skyColor.rgb * tint, 1.0);
}
