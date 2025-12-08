#version 330 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNorm;
layout(location=2) in vec2 aTex;

out vec2 vTex;

uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;

void main(){
    vTex = aTex;
    vec4 pos = uProjection * uView * uModel * vec4(aPos, 1.0);
    gl_Position = pos;
}
