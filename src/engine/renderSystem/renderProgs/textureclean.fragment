uniform sampler2D	u_TextureMap;

varying vec2		var_TexCoords;
varying vec2		var_Dimensions;

uniform vec4		u_ViewInfo; // zfar / znear, zfar
uniform vec2		u_Dimensions;

varying vec4		var_Local0;

//#define MSIZE var_Local0.b
#define MSIZE 4

float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

float normpdf3(in vec3 v, in float sigma)
{
	return 0.39894*exp(-0.5*dot(v,v)/(sigma*sigma))/sigma;
}


void main(void)
{
	float SIGMA = var_Local0.r;
	float BSIGMA = var_Local0.g;
	vec2 coord = var_TexCoords;
	vec2 offset = vec2(1.0 / var_Dimensions.s, 1.0 / var_Dimensions.t);

	vec3 c = texture2D(u_TextureMap, coord - (vec2(0.0, 1.0) * offset)).rgb;

	//declare stuff
	const int kSize = (MSIZE-1)/2;
	float kernel[MSIZE];
	vec3 final_colour = vec3(0.0);
		
	//create the 1-D kernel
	float Z = 0.0;

	for (int j = 0; j <= kSize; ++j)
	{
		kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), SIGMA);
	}
		
	vec3 cc;
	float factor;
	float bZ = 1.0/normpdf(0.0, BSIGMA);

	//read out the texels
	for (int i=-kSize; i <= kSize; ++i)
	{
		for (int j=-kSize; j <= kSize; ++j)
		{
			vec2 offset2 = vec2(float(i),float(j)) * offset;
			cc = texture2D(u_TextureMap, (coord + offset2) - (vec2(0.0, 1.0) * offset)).rgb;
			factor = normpdf3(cc-c, BSIGMA)*bZ*kernel[kSize+j]*kernel[kSize+i];
			Z += factor;
			final_colour += factor*cc;
		}
	}
		
	gl_FragColor = vec4(final_colour/Z, 1.0);
}
