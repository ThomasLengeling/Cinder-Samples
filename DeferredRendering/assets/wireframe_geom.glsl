#version 120
#extension GL_EXT_gpu_shader4 : enable
#extension GL_EXT_geometry_shader4 : enable

// required for ATI GPU's:
//layout( triangles ) in;
//layout( triangle_strip, max_vertices = 3 ) out;

noperspective varying vec3	vDistance;

uniform vec2 				uViewportSize;

void main(void)
{
	// taken from 'Single-Pass Wireframe Rendering'
	vec2 p0 = uViewportSize * gl_PositionIn[0].xy/gl_PositionIn[0].w;
	vec2 p1 = uViewportSize * gl_PositionIn[1].xy/gl_PositionIn[1].w;
	vec2 p2 = uViewportSize * gl_PositionIn[2].xy/gl_PositionIn[2].w;

	vec2 v0 = p2-p1;
	vec2 v1 = p2-p0;
	vec2 v2 = p1-p0;
	float fArea = abs(v1.x*v2.y - v1.y * v2.x);

	vDistance = vec3(fArea/length(v0),0,0);
	gl_Position = gl_PositionIn[0];
	EmitVertex();

	vDistance = vec3(0,fArea/length(v1),0);
	gl_Position = gl_PositionIn[1];
	EmitVertex();

	vDistance = vec3(0,0,fArea/length(v2));
	gl_Position = gl_PositionIn[2];
	EmitVertex();

	EndPrimitive();
}