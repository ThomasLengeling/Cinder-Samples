#version 110

varying vec4	vVertex;
varying vec3	vNormal;
varying vec3	vTangent;
varying vec3	vBiTangent;

void main()
{	
	// view space position
	vVertex = gl_ModelViewMatrix * gl_Vertex; 

	// view space normal, tangent and bitangent 
	vNormal = normalize(gl_NormalMatrix * gl_Normal);
	vTangent = normalize(gl_NormalMatrix * gl_MultiTexCoord7.xyz); 
	vBiTangent = normalize(cross(vNormal, vTangent));

	// texture coordinates and screen space position
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}
