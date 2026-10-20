#version 330 core
uniform sampler2D layer;
out vec4 FragColor;

void main()
{
    FragColor = texture(layer, gl_FragCoord.xy / vec2(textureSize(layer, 0)));
}
