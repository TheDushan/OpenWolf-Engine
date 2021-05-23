attribute vec3 attr_Position;
attribute vec4 attr_TexCoord0;

uniform mat4   u_ModelViewProjectionMatrix;

uniform vec2	u_Dimensions;

varying vec2   var_TexCoords;
varying vec2   var_Dimensions;

void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;
	var_Dimensions = u_Dimensions.st;
}
