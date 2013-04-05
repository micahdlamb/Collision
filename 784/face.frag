#version 330 core

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
	float time;
	float deltaTime;
	vec2 screenDim;
};

layout(std140) uniform Object {
	mat4 worldTransform;
	mat3 normalTransform;
};

uniform sampler2D normals;
uniform sampler2D colors;
uniform sampler2D irradiance;
uniform sampler2D gauss0;
uniform sampler2D gauss1;
uniform sampler2D gauss2;
uniform sampler2D gauss3;
uniform sampler2D gauss4;
uniform sampler2D gauss5;
uniform sampler2D specular;
uniform sampler2D beckman;
uniform sampler3D perlin;
uniform samplerCube reflections;
uniform int reflectionsOn;
uniform int phongOn;

in vec3 localPos;
in vec3 worldPos;
in vec2 uv;
//in vec2 uv0;
//in vec2 uv1;

out vec4 outColor;

float fresnelReflectance( vec3 H, vec3 V, float F0 ){
	float base = 1.0 - dot( V, H );
	float exponential = pow( base, 5.0 );
	return exponential + F0 * ( 1.0 - exponential );
}

float KS_Skin_Specular( vec3 N, // Bumped surface normal
	vec3 L, // Points to light
	vec3 V, // Points to eye
	float m,  // Roughness
	float rho_s) // Specular brightness
{
	float result = 0.0;
	float ndotl = dot( N, L );
	if( ndotl > 0.0 ){
		vec3 h = L + V; // Unnormalized half-way vector
		vec3 H = normalize( h );
		float ndoth = dot( N, H );
		float PH = pow( 2.0 * texture(beckman,vec2(ndoth,m)).r, 10.0 );
		float F = fresnelReflectance( H, V, 0.028 );
		float frSpec = max( PH * F / dot( h, h ), 0 );
		result = ndotl * rho_s * frSpec; // BRDF * dot(N,L) * rho_s
	}
	return result;
}

void main(void)
{
	/*
	vec2 uv = vec2(
		( fwidth( uv0.x ) < fwidth( uv1.x )-0.001 )? uv0.x : uv1.x
		,( fwidth( uv0.y ) < fwidth( uv1.y )-0.001 )? uv0.y : uv1.y
	);
	*/

	//hardcoded for now
	float amb = .2;
	float shininess = 30;
	vec3 norm = texture(normals, uv).rgb*2-vec3(1);
	norm = normalize(normalTransform * norm);


	norm += vec3(
		texture(perlin, localPos.xyz*.05 + vec3(.5)).r
		,texture(perlin, localPos.zxy*.05 + vec3(.5)).r
		,texture(perlin, localPos.yzx*.05 + vec3(.5)).r
	)*.05;


	if (!gl_FrontFacing) norm *= -1;

	//non blurred irradiance
	vec3 nondiffused = texture(irradiance, uv).rgb;
	//blurred irradiance
	vec3 diffused =
		texture(gauss0, uv).rgb * vec3(.233, .455, .649)
		+ texture(gauss1, uv).rgb * vec3(.1, .336, .344)
		+ texture(gauss2, uv).rgb * vec3(.118, .198, 0)
		+ texture(gauss3, uv).rgb * vec3(.113, .007, .007)
		+ texture(gauss4, uv).rgb * vec3(.358, .004, 0)
		+ texture(gauss5, uv).rgb * vec3(.078, 0, 0);

	//Color the surface
	vec3 diffuseColor = texture(colors, uv).rgb;
	vec3 lightColor = lights[0].color;
	vec3 lightPos = lights[0].pos;
	vec3 lightDir = normalize(lightPos - worldPos);
	vec3 eyeDir = normalize(eyePos - worldPos);
	vec3 reflectDir = reflect(-lightDir, norm);

	//outColor = texture(beckman,worldPos.xy);
	//return;

	
	vec4 s = texture(specular, uv);//not sure what all the components represent
	//float m = s.w * .09 + .23;
	float m = .3;//from http://developer.download.nvidia.com/presentations/2007/gdc/Advanced_Skin.pdf
	float rho_s = s.x * .16 + .18;//copied from advanced skin - final.cg, line 203
	float spec = KS_Skin_Specular(norm, lightDir, eyeDir, m, rho_s);
	//outColor = vec4(spec,spec,spec,1);
	//return;


	//ambient
	vec3 color = amb * diffuseColor * 0;
	//diffuse
	color += phongOn==1 ? nondiffused : diffused;
	//specular
	//color += lightColor * spec * pow(max(0.0, dot(reflectDir, eyeDir)), shininess);
	color += lightColor * spec;
	
	if (reflectionsOn == 1)
		color = mix(color, texture(reflections, reflect(-eyeDir, norm)).rgb, .2);

	outColor = vec4(color,1);
}