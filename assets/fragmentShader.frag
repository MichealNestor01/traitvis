#version 330 core

in vec3 vertexColour;

layout(location=0) out vec3 outputColour;

void main() {
    outputColour = vertexColour;
}