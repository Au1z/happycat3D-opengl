#version 330 core
in vec2 vTex;
out vec4 FragColor;

uniform sampler2D uSkyTex;

void main(){
    vec3 color = texture(uSkyTex, vTex).rgb;
    FragColor = vec4(color, 1.0);
}