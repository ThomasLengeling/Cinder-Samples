#version 120

uniform sampler2D	uSSAOMap;
uniform vec4		uScreenParams;
uniform vec2		uDirection;

const int			kKernelWidth = 7;

void main()
{
	const int iHalfKernelWidth = (kKernelWidth-1) / 2;
    const float fSigma = (kKernelWidth-1) / iHalfKernelWidth;
    
    float fLinearDepth = texture2D(uSSAOMap, gl_TexCoord[0].st).g;
	
    float fWeights = 0;
    float fBlurred = 0;
    
	for (float i=-iHalfKernelWidth; i<iHalfKernelWidth; i++)
	{
		vec2  vOffset = (i * uDirection) * uScreenParams.zw;
		vec2  vSample = texture2D(uSSAOMap, gl_TexCoord[0].st + vOffset).rg;

		float fSampleDepth = vSample.y;
		float fGeometricWeight = exp(-pow(i, 2.0) / (2.0 * pow(fSigma, 2.0)));
		float fPhotometricWeight = 1.0 / (1.0 + abs(fLinearDepth - fSampleDepth));

		float fFactor = fGeometricWeight * fPhotometricWeight;
		fWeights += fFactor;
		fBlurred += vSample.x * fFactor;
	}

    gl_FragColor.r = fBlurred / fWeights;
	gl_FragColor.g = fLinearDepth;
}