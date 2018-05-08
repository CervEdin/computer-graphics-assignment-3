// Fragment shader
#version 150

in vec3 v_color;

in vec3 N;
in vec3 L;
in vec3 V;

uniform vec3 ambient_color;
uniform vec3 diffuse_color;
uniform vec3 specular_color;
uniform float specular_power;

out vec4 frag_color;

void main()
{
    frag_color = vec4(v_color, 1.0);
}
