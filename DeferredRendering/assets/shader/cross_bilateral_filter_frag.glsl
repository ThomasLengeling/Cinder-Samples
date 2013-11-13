#version 120

uniform sampler2D	uSSAOMap;
uniform vec4		uScreenParams;

const float uPhotometricExponent = 2.0;

void main()
{
    const int iKernelWidth = 7;
	const int iHalfKernelWidth = (iKernelWidth-1) / 2;
    const float fSigma = (iKernelWidth-1) / iHalfKernelWidth;
    
    float fDepth = texture2D(uSSAOMap, gl_TexCoord[0].st).g;
	
    float fWeights = 0;
    float fBlurred = 0;
    
	for (float j=-iHalfKernelWidth; j<iHalfKernelWidth; j++)
	{	
		float fGeometricWeightY = exp(-pow(j, 2.0) / (2.0 * pow(fSigma, 2.0)));

		for (float i=-iHalfKernelWidth; i<iHalfKernelWidth; i++)
		{
			vec2  vOffset = vec2(i,j) * uScreenParams.zw;
			vec2  vSample = texture2D(uSSAOMap, gl_TexCoord[0].st + vOffset).rg;

			float fSampleDepth = vSample.y;
			float fGeometricWeight = exp(-pow(i, 2.0) / (2.0 * pow(fSigma, 2.0))) * fGeometricWeightY;
			float fPhotometricWeight = 1.0 / pow(1.0 + abs(fDepth - fSampleDepth), uPhotometricExponent);

			float fFactor = fGeometricWeight * fPhotometricWeight;
			fWeights += fFactor;
			fBlurred += vSample.x * fFactor;
		}
	}

    gl_FragColor.r = fBlurred / fWeights;
	//gl_FragColor.g = fDepth;
}