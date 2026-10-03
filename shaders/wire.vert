#version 410 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;

out float vDepth;
out vec3 vObject;
out vec2 vNdc;

void main(){
  vec4 p=uMVP*vec4(aPos,1.0);
  gl_Position=p;
  vObject=aPos;
  vNdc=p.xy/max(abs(p.w),1e-5);
  vDepth=clamp(1.0-abs(p.z/p.w)*0.16,0.15,1.0);
}
