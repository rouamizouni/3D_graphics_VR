#version 450 core

layout(location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 prevModel;   // model matrix from previous frame
uniform mat4 prevView;    // view matrix from previous frame

// Clip-space positions for current and previous frame
out vec4 currClipPos;
out vec4 prevClipPos;

void main()
{
    vec4 worldPos     = model     * vec4(position, 1.0);
    vec4 prevWorldPos = prevModel * vec4(position, 1.0);

    currClipPos = projection * view     * worldPos;
    prevClipPos = projection * prevView * prevWorldPos;

    gl_Position = currClipPos;
}
