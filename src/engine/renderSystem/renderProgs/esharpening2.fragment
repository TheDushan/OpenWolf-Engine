uniform sampler2D u_TextureMap;

uniform vec4	u_ViewInfo; // zfar / znear, zfar
uniform vec2	u_Dimensions;

varying vec2   var_TexCoords;
varying vec4	var_ViewInfo; // zfar / znear, zfar
varying vec2   var_Dimensions;
varying vec4   var_Local0; // num_passes, 0, 0, 0

vec2 texCoord = var_TexCoords;

float near = u_ViewInfo.x;
float far = u_ViewInfo.y;
float viewWidth = var_Dimensions.x;
float viewHeight = var_Dimensions.y;

float width = viewWidth;
float height = viewHeight;

#define NbPixel     1
#define Edge_threshold  0.2
#define Sharpen_val0    2.0
#define Sharpen_val1    0.125

void main()
{
	vec2 tex = var_TexCoords.xy;

	// size of NbPixel pixels
	float dx = NbPixel / width;
	float dy = NbPixel / height;
	vec4 Res = vec4(0.0, 0.0, 0.0, 0.0);

	// Edge detection using Prewitt operator
	// Get neighbor points
	// [ 1, 2, 3 ]
	// [ 4, 0, 5 ]
	// [ 6, 7, 8 ]
	vec4 c0 = texture2D(u_TextureMap, tex);
	vec4 c1 = texture2D(u_TextureMap, tex + vec2(-dx, -dy));
	vec4 c2 = texture2D(u_TextureMap, tex + vec2(  0, -dy));
	vec4 c3 = texture2D(u_TextureMap, tex + vec2( dx, -dy));
	vec4 c4 = texture2D(u_TextureMap, tex + vec2(-dx,   0));
	vec4 c5 = texture2D(u_TextureMap, tex + vec2( dx,   0));
	vec4 c6 = texture2D(u_TextureMap, tex + vec2(-dx,  dy));
	vec4 c7 = texture2D(u_TextureMap, tex + vec2(  0,  dy));
	vec4 c8 = texture2D(u_TextureMap, tex + vec2( dx,  dy));

	// Computation of the 3 derived vectors (hor, vert, diag1, diag2)
	vec4 delta1 = (c6 + c4 + c1 - c3 - c5 - c8);
	vec4 delta2 = (c4 + c1 + c2 - c5 - c8 - c7);
	vec4 delta3 = (c1 + c2 + c3 - c8 - c7 - c6);
	vec4 delta4 = (c2 + c3 + c5 - c7 - c6 - c4);

	// Computation of the Prewitt operator
	float value = length(abs(delta1) + abs(delta2) + abs(delta3) + abs(delta4)) / 6;

	// If we have an edge (vector length > Edge_threshold) => apply sharpen filter
	if (value > Edge_threshold) {
		Res = c0 * Sharpen_val0 - (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8) * Sharpen_val1;
		// Display edges in red...
		//Res = vec4( 1.0, 0.0, 0.0, 0.0 );

		gl_FragColor = Res;
	} else {
		gl_FragColor = c0;
	}
}
