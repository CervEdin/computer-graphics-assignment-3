// Fragment shader
#version 150

in vec3 N;
in vec3 L;
in vec3 V;

uniform vec3 u_pos_light_pos;
uniform vec3 u_pos_light_col;
uniform vec3 u_ambient_color;
uniform vec3 u_diffuse_color;
uniform vec3 u_specular_color;
uniform float u_specular_power;

out vec4 frag_color;

void main()
{
	// implement normalized blinn-phong shading model
	vec3 normal = normalize(N);
	//vec3 reflection = reflect(-V, N);
	vec3 half_way = normalize(L+V);
	vec3 illumination_ambient = u_ambient_color;
	vec3 illumination_diffuse = u_diffuse_color * u_pos_light_col * max(dot(N, L), 0);
	vec3 illumination_specular = u_specular_power * u_specular_color * u_pos_light_col
	* pow(
		max(dot(N, half_way), 0.0)
		, u_specular_power);
	vec3 color = illumination_ambient;
	color += illumination_diffuse;
	color += illumination_specular;
    frag_color = vec4(color, 1.0);
}
