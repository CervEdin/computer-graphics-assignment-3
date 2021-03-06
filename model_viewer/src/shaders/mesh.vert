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

out vec3 v_normal;
out vec3 v_light;
out vec3 v_view;

void main()
{
    gl_Position = u_mvp * a_position;

	// Transform the vertex position to view space (eye coordinates)
	vec3 position_eye = vec3(u_mv * a_position);

	// Calculate the view-space normal
	v_normal = normalize(mat3(u_mv) * a_normal);

	// Calculate the view-space light direction
	v_light = normalize(u_pos_light_pos - position_eye);

	// Calculate the view-vector 
	v_view = normalize(-position_eye);

	// Calculate the diffuse (Lambertian) reflection term
	//float diffuse = max(0.0, dot(v_normal, v_light));
}
