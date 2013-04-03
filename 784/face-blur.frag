#version 330 core
in vec2 uv;

uniform vec2 scale;
uniform sampler2D tex;
uniform sampler2D stretch;

out vec4 outColor;


//from http://developer.download.nvidia.com/presentations/2007/gdc/Advanced_Skin.pdf slide 98
vec4 gauss[] = vec4[]
(
	vec4(.042,.22,.437,.635)
	,vec4(.22,.101,.355,.365)
	,vec4(.433,.119,.208,0)
	,vec4(.753,.114,0,0)
	,vec4(1.412,.364,0,0)
	,vec4(2.722,.080,0,0)
);

void main()
{
	//stretch is 1 / dist in world space that each pixel covers
	//scale is the dist in texture space that each pixel covers
	vec2 stretch = texture(stretch, uv).rg;

	//texture space dist / world dist
	vec2 ratio = stretch * scale;

	//outColor = vec4(ratio*1024

	//outColor = vec4(stretch,0,1);
	//return;
	
	//vec2 width = scale * stretch * GaussWidth;//not sure what to use for gaussWidth
	vec3 color = vec3(0);
	for (int i = -1; i < 2; i += 2)
		for( int j = 0; j < 6; j++ ){
			vec4 x = gauss[j];
			vec2 offset = i * x.r * ratio * 10;//10 converts 1e-4 (the units of stretch) to millimeters
			vec3 irr = texture( tex, uv + offset ).rgb;
			color += irr * x.gba * .5;
		}

	outColor = vec4(color,1);
}

/*orig
	vec3 color = vec3(0);
	for( int i = -5; i <= 5; i++ ){
		vec4 fil = gauss[abs(i)];

		vec3 irr = texture( tex, vec2( uv.x + i * scale.x, uv.y + i * scale.y ) ).rgb;
		float w = (i==0 ? 1 : .5);//since all but i==0 get added twice
		color += irr * fil.gba * w;
	}

	outColor = vec4(color,1);
*/