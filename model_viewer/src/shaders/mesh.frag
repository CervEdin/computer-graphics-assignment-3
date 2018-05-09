// Fragment shader
#version 150

in vec3 v_normal;
in vec3 v_light;
in vec3 v_view;

uniform vec3 u_pos_light_pos;
uniform vec3 u_pos_light_col;
uniform vec3 u_ambient_color;
uniform vec3 u_diffuse_color;
uniform vec3 u_specular_color;
uniform float u_specular_power;

uniform int u_ambient_on;
uniform int u_diffuse_on;
uniform int u_specular_on;
uniform int u_gamma_on;
uniform int u_surface_normal_rgb_on;

uniform samplerCube u_cubemap;

out vec4 frag_color;

void main()
{
	// implement normalized blinn-phong shading model
	vec3 normal = normalize(v_normal);
	vec3 reflection = reflect(-v_view, v_normal);
	vec3 half_way = normalize(v_light + v_view);
	vec3 illumination_ambient = u_ambient_color * u_ambient_on;
	vec3 illumination_diffuse = u_diffuse_color * u_pos_light_col * max(dot(v_normal, v_light), 0) * u_diffuse_on;
	float specAngle = max(dot(v_normal, half_way), 0.0);
	vec3 illumination_specular = u_specular_color * u_pos_light_col * pow(specAngle, u_specular_power) * u_specular_on;
	vec3 color = texture(u_cubemap, reflection).rgb;
    color += illumination_ambient;
	color += illumination_diffuse;
	color += illumination_specular;
	if (u_surface_normal_rgb_on == 1) {
	    color = v_normal;
	}
	if (u_gamma_on == 1) {
    	color = pow(color, vec3(1.0/2.2)); // sRGB gamma correction
    }
    frag_color = vec4(color, 1.0);
}
