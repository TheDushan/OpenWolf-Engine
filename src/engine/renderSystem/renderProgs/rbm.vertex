attribute vec3				attr_Position;
attribute vec2				attr_TexCoord0;

uniform mat4				u_ModelViewProjectionMatrix;
uniform vec2				u_Dimensions;
uniform vec4				u_ViewInfo; // zmin, zmax, zmax / zmin
uniform vec3				u_ViewOrigin;

varying vec2				var_ScreenTex;
varying vec2				var_Dimensions;
varying vec3				var_ViewDir;

void main()
{
	var_ScreenTex = attr_TexCoord0.xy;
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_Dimensions = u_Dimensions;
	var_ViewDir = u_ViewOrigin - attr_Position.xyz;
}
