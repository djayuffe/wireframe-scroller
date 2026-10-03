#version 410 core
in vec2 uv;
out vec4 FragColor;
uniform sampler2D uTexA;
uniform sampler2D uTexB;
uniform float uMix;            // 0 = A, 1 = B (crossfade)
uniform float uScaleA;         // Ken Burns zoom for A
uniform float uScaleB;
uniform float uOpacity;         // overall logo opacity
uniform float uTime;
uniform float uMusicLevel;
// cover-fit: crop the logo to the screen aspect, keep full frame.
vec2 cover(vec2 uv,vec2 texAspect,vec2 screenAspect){
  // scale the uv so the texture covers the screen.
  float r=texAspect.x/texAspect.y;
  float s=screenAspect.x/screenAspect.y;
  if(r>s){ // texture wider: crop sides
    uv.x=(uv.x-0.5)*(s/r)+0.5;
  } else { // texture taller: crop top/bottom
    uv.y=(uv.y-0.5)*(r/s)+0.5;
  }
  return uv;
}
void main(){
  vec2 sa=cover(uv,vec2(uScaleA,1.0),vec2(1920.0,1080.0));
  vec2 sb=cover(uv,vec2(uScaleB,1.0),vec2(1920.0,1080.0));
  vec3 a=texture(uTexA,sa).rgb;
  vec3 b=texture(uTexB,sb).rgb;
  vec3 col=mix(a,b,clamp(uMix,0.0,1.0));
  // very subtle brightness breathing with the beat/music.
  float br=0.82+0.10*clamp(uMusicLevel,0.0,1.0)+0.03*sin(uTime*0.5);
  col*=br;
  FragColor=vec4(col*uOpacity,uOpacity);
}
