#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in mat4 aInstanceModel;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool invertNormal;
uniform bool useInstanceModel;

out vec3 Normal;
out vec3 FragPos;

void main()
{
	mat4 m = useInstanceModel ? aInstanceModel : model;
	gl_Position = projection * view * m * vec4(aPos, 1.0);
	Normal = mat3(transpose(inverse(m))) * aNormal * (invertNormal ? -1.0 : 1.0);
	FragPos = vec3(m * vec4(aPos, 1.0));
}