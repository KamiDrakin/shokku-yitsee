#version 100

// Input vertex attributes
attribute vec3 vertexPosition;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;
attribute vec2 vertexTexCoord;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matView;
uniform vec4 colDiffuse;

// Output vertex attributes (to fragment shader)
varying vec4 fragColor;
varying vec2 fragTexCoord;

varying vec3 frag_light_pos;
varying vec3 frag_normal;
uniform vec3 light_src;

void main() {
  vec4 pos = mvp * vec4(vertexPosition, 1.0);
  if (length(vertexNormal) != 0.0) {
    frag_light_pos = (matView * vec4(light_src, 1.0) - matView * matModel * vec4(vertexPosition, 1.0)).xyz;
    frag_normal = (matView * vec4(normalize(vertexNormal), 0.0)).xyz;
  }
  else {
    frag_light_pos = (matView * vec4(light_src, 1.0) - matView * vec4(vertexPosition, 1.0)).xyz;
    frag_normal = vec3(0.0, 0.0, 1.0);
  }
  fragColor = vertexColor * colDiffuse;
  fragTexCoord = vertexTexCoord;
  gl_Position = pos;
}