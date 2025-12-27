#extension GL_EXT_geometry_shader4: enable

layout( triangles ) in;
layout( triangle_strip, max_vertices=3 ) out;

vec3 V[3];
vec3 CG;

void ProduceVertex( int v ) {
	gl_Position = vec4( CG + 0.95f * ( V[v] - CG ), 1.0f );
	EmitVertex( );
}

void main() {
	V[0] = gl_PositionIn[0].xyz;
	V[1] = gl_PositionIn[1].xyz;
	V[2] = gl_PositionIn[2].xyz;
	CG = ( V[0] + V[1] + V[2] ) / 3.0f;
	ProduceVertex( 0 );
	ProduceVertex( 1 );
	ProduceVertex( 2 );
}