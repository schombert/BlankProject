in float adjusted_b;

uniform sampler2D line_texture;
uniform samplerBuffer curves;

out vec4 frag_color;

void main() {
	frag_color = vec4(1.0f, 1.0f - adjusted_b, adjusted_b, 0.2f);
}
