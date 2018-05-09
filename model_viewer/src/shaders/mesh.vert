// Vertex shader
#version 150
#extension GL_ARB_explicit_attrib_location : require

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_mvp;
uniform mat4 u_mv;

// Uniform variables needed in frag
uniform vec3 u_pos_light_pos;
uniform vec3 u_pos_light_col;
uniform vec3 u_ambient_color;
uniform vec3 u_diffuse_color;
uniform vec3 u_specular_color;
uniform float u_specular_power;

out vec3 N;
out vec3 L;
out vec3 V;

out vec3 v_color;

void main()
{
    gl_Position = u_mvp * a_position;

	// Transform the vertex position to view space (eye coordinates)
	vec3 position_eye = vec3(u_mv * a_position);

	// Calculate the view-space normal
	N = normalize(mat3(u_mv) * a_normal);

	// Calculate the view-space light direction
	L = normalize(u_pos_light_pos - position_eye);

	// Calculate the view-vector 
	V = normalize(-position_eye);

	// Calculate the diffuse (Lambertian) reflection term
	float diffuse = max(0.0, dot(N, L));

	// Multiply the diffuse reflection term with the surface color
	v_color = diffuse * vec3(0.0, 1.0, 0.0);
}
