#version 410 core

layout (location = 0) in vec2 vPos;

layout(std140) uniform Object {
	mat4 worldTransform;
	mat4 normalTransform;
};

uniform sampler2D heightMap;

out vec3 tcWorldPos;
out vec2 tcTexCoords;

void main()
{
	tcTexCoords = vPos.xy;
	float y = texture(heightMap, tcTexCoords.yx).x;
	tcWorldPos = (worldTransform * vec4(vec3(vPos.x,y,vPos.y), 1)).xyz;
}