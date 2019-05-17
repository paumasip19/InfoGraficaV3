#version 330

in vec3 FragPos;
in vec3 vert_Normal;

out vec4 out_Color;

uniform vec3 lightPos;
uniform vec3 cameraPoint;
uniform vec3 objectColor;
uniform vec3 lightColor;

uniform vec3 moonPos;
uniform vec3 moonColor;

vec3 Light(vec3 _lightPosition, vec3 _lightColor, float dLength = 1000)
{
	//ambient
	float ambientStrength = 0.5f;
	vec3 ambient = ambientStrength * _lightColor;

	//diffuse
	vec3 norm = normalize(vert_Normal);
	vec3 lightDir = _lightPosition - FragPos;
	float lightLength = length(lightDir);
	vec3 lightDirNorm = normalize(lightDir);
	
	float distForce = 1;
	if(lightLength > dLength)distForce = 0;
	
	float diff = max(dot(norm, lightDirNorm), 0.0);
	if(diff < 0.2) diff = 0;
	if(diff >= 0.2 && diff < 0.4) diff = 0.2;
	if(diff >= 0.4 && diff < 0.5) diff = 0.4;
	if(diff >= 0.5) diff = 1;
	vec3 diffuse = diff * _lightColor;

	//specular
	float specularStrength = 1.0f;
	vec3 viewDir = normalize(cameraPoint - FragPos);
	vec3 reflectDir = reflect(-lightDirNorm, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * _lightColor;

	return (ambient + diffuse + specular) * objectColor * distForce;
}

void main() 
{

	vec3 result = Light(lightPos, lightColor, 1) + Light(moonPos, moonColor);

	out_Color = vec4(result, 1.0f);
}