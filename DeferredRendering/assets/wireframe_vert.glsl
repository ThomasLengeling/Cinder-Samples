#version 120
#extension GL_EXT_gpu_shader4 : enable

void main() {
	// pass texture coordinates and screen space position
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}