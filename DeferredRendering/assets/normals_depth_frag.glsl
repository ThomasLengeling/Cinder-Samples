#version 110

varying vec4		vVertex;
varying vec3		vNormal;
varying vec3		vTangent;
varying vec3		vBiTangent;

uniform bool		uHasNormalMap;
uniform	sampler2D	uNormalMap;

vec2 encode (vec3 n)
{
    vec2 enc = normalize(n.xy) * (sqrt(-n.z*0.5+0.5));
    return (enc*0.5 + 0.5);
}

void main()
{		 
	// calculate view space surface normal
	vec3 vSurfaceNormal = vNormal;

	if(uHasNormalMap)
	{
		vec3 vMappedNormal = texture2D(uNormalMap, gl_TexCoord[0].st).rgb * 2.0 - 1.0;
		vSurfaceNormal = normalize((vTangent * vMappedNormal.x) + (vBiTangent * vMappedNormal.y) + (vNormal * vMappedNormal.z));
	}

	// encode normal as sphere map and output to buffer
	gl_FragColor.rg	= encode(vSurfaceNormal);
}