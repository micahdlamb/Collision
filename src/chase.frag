#version 330

struct Light {
	mat4 eye;
	vec3 pos;
	vec3 color;
	vec4 pad[2];
};

layout(std140) uniform Global {
	mat4 eye;
	vec3 eyePos;
	int numLights;
	Light lights[6];
};

uniform sampler2D alphaMask;

in vec2 coord;
in vec3 fColor;
in float radius;
in vec3 center;
in vec3 toEye;
in vec3 right;
in vec3 up;

out vec4 fragColor;

void main()
{
	vec2 c = (coord - .5) * 2;//-1 -> 1
	if (length(c) > 1)
		discard;

	c = c * radius;

	float z = sqrt(radius*radius - c.x*c.x - c.y*c.y);

	vec3 worldPos = center + c.x * right * .5 + c.y * up * .5 + z * toEye;
	vec3 norm = normalize(worldPos - center);

	float amb = .25;
	float dif = .4;
	float ddif = .8;
	float spec = .25;
	float shininess = 30;

	vec3 lightColor = lights[0].color;
	vec3 lightPos = lights[0].pos;
	vec3 lightDir = normalize(lightPos - worldPos);
	vec3 eyeDir = normalize(eyePos - worldPos);
	vec3 reflectDir = reflect(-lightDir, norm);

	//ambient
	vec3 color = fColor * amb;
	//diffuse
	//color += fColor * dif * max(0.0, dot(norm, lightDir));
	//directional diffuse
	color += fColor * ddif * max(0.0, dot(norm, vec3(0,1,0)));
	//specular
	//color += lightColor * spec * pow(max(0.0, dot(reflectDir, eyeDir)), shininess);

	vec4 projected = eye * vec4(worldPos,1);
	gl_FragDepth = (projected.z / projected.w + 1) / 2;
	fragColor = vec4(color,1);
}
