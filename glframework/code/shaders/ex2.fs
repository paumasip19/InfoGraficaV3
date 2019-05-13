#version 330

in vec3 FragPos;
in vec3 vert_Normal;

out vec4 out_Color;

uniform vec3 lightPos;
uniform vec3 cameraPoint;
uniform vec3 objectColor;
uniform vec3 lightColor;


void main() 
{	
	//ambient
	float ambientStrength = 0.5f;
	vec3 ambient = ambientStrength * lightColor;

	//diffuse
	vec3 norm = normalize(vert_Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0f);
	vec3 diffuse = diff * lightColor;

	//specular
	float specularStrength = 1.0f;
	vec3 viewDir = normalize(cameraPoint - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor;

	vec3 result = (ambient + diffuse + specular) * objectColor;

	out_Color = vec4(result, 1.0f);
}