#version 330 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
uniform vec2 viewportSize;
out vec4 vertexColor;
void main()
{
    vec2 clip = position / viewportSize * 2.0 - 1.0;
    gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);
    vertexColor = color;
}
