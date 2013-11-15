#version 120

const int			MAX_KERNEL_SIZE = 256;
const float			uRadius = 0.25;
const float			uPower = 1.2;

uniform sampler2D	uGBuffer;
uniform sampler2D	uGBufferDepth;
uniform vec2		uGBufferSize;

uniform vec4		uProjectionParams;
uniform mat4		mProjectionMatrix;

uniform sampler2D	uNoise;
uniform vec2		uNoiseSize;
uniform int			uKernelSize;
uniform vec3		uKernelOffsets[MAX_KERNEL_SIZE];

vec3 decode (in vec2 enc)
{
    vec4 nn = vec4(enc, 0, 0) * vec4(2, 2, 0, 0) + vec4(-1, -1, 1, -1);
    float l = dot(nn.xyz, -nn.xyw);
    nn.z = l;
    nn.xy *= sqrt(l);
    return nn.xyz * 2.0 + vec3(0, 0, -1);
}

float linearizeDepth(in float depth) {
	return uProjectionParams.w / (depth - uProjectionParams.z);
}

vec3 getViewPositionFromLinearDepth(in float depth, in vec2 ndc)
{
	return vec3((ndc * depth) / uProjectionParams.xy, depth);
}

float calculateOcclusion(in mat3 mKernel, in vec3 vOrigin, in float fRadius) 
{
	float fOcclusion = 0.0;
	for (int i = 0; i < uKernelSize; ++i) 
	{
		//	get sample position:
		vec3 vKernelOffset = mKernel * uKernelOffsets[i];
		vec3 vSamplePos = vOrigin - fRadius * vKernelOffset;
		
		//	project sample position:
		vec4 vOffset = mProjectionMatrix * vec4(vSamplePos, 1.0);
		vOffset.xy /= vOffset.w;
		vOffset.xy = 0.5 - vOffset.xy * 0.5;
		
		//	get sample depth:
		float fDepth = texture2D( uGBufferDepth, vOffset.xy ).r;
		float fLinearDepth = linearizeDepth( fDepth );
		
		float fRangeCheck = smoothstep(0.0, 1.0, fRadius / abs(vOrigin.z - fLinearDepth));
		fOcclusion += fRangeCheck * step(fLinearDepth, vSamplePos.z);
	}
	
	fOcclusion = 1.0 - fOcclusion / float(uKernelSize);
	return pow(fOcclusion, uPower);
}

void main()
{	
	vec4	vNormalAndSpecularPower =  texture2D( uGBuffer, gl_TexCoord[0].st );

	// calculate view space fragment position from depth
	float	fDepth = texture2D( uGBufferDepth, gl_TexCoord[0].st ).r;
	float	fLinearDepth = linearizeDepth( fDepth );
	vec2	vNormalizedCoords = gl_TexCoord[0].st * 2.0 - 1.0;
	vec3	vPosition = getViewPositionFromLinearDepth( fLinearDepth, vNormalizedCoords );
	
	// decode view space surface normal
	vec2	vEncodedNormal = vNormalAndSpecularPower.rg;
	vec3	vSurfaceNormal = decode( vEncodedNormal );

	// create sample ray
	vec2	vNoiseTexCoords = uGBufferSize / uNoiseSize * gl_TexCoord[0].st;
	vec3	vRay = texture2D( uNoise, vNoiseTexCoords ).rgb * 2.0 - 1.0;

	// create sample kernel
	vec3	vTangent = normalize(vRay - vSurfaceNormal * dot(vRay, vSurfaceNormal));
	vec3	vBiTangent = cross(vTangent, vSurfaceNormal);
	mat3	mKernel = mat3(vTangent, vBiTangent, vSurfaceNormal);
	
	// perform SSAO and output to buffer
	gl_FragColor.r = calculateOcclusion(mKernel, vPosition, uRadius);
	gl_FragColor.g = fLinearDepth;
}