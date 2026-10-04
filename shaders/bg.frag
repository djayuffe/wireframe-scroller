#version 410 core
in vec2 uv;
out vec4 FragColor;
uniform sampler2D uTexA;
uniform sampler2D uTexB;
uniform float uMix;        // 0 = A, 1 = B (crossfade)
uniform float uZoom;       // Ken Burns zoom (>=1)
uniform float uAspect;     // screen aspect (w/h)
uniform vec2 uTexAspect;   // (A and B share the same 16:9 aspect)
uniform float uOpacity;    // overall logo opacity
uniform float uTime;
uniform float uMusicLevel;
// Cover-fit: crop the logo so it fills the screen (no letterbox).
vec2 coverUv(vec2 uv, float texAR, float scrAR){
  // texAR = texture aspect, scrAR = screen aspect.
  vec2 c=vec2(1.0);
  if(scrAR>texAR){ c.x=texAR/scrAR; }   // screen wider -> crop top/bottom
  else           { c.y=scrAR/texAR; }   // screen taller -> crop sides
  return (uv-0.5)*c+0.5;
}
void main(){
  float zoom=1.0/uZoom;                 // zoom in
  vec2 a=(uv-0.5)/zoom+0.5;
  vec2 b=(uv-0.5)/zoom+0.5;
  vec2 ua=coverUv(a,uTexAspect.x/uTexAspect.y,uAspect);
  vec2 ub=coverUv(b,uTexAspect.x/uTexAspect.y,uAspect);
  vec3 col=mix(texture(uTexA,ua).rgb,texture(uTexB,ub).rgb,clamp(uMix,0.0,1.0));
  // gentle brightness breathing with the beat.
  col*=0.92+0.08*clamp(uMusicLevel,0.0,1.0)+0.02*sin(uTime*0.5);
  // Straight (non-premultiplied) RGB; the blend (SRC_ALPHA/ONE_MINUS_SRC_ALPHA)
  // applies the opacity. In logo mode the logo is drawn opaque (no blend) as
  // the backdrop, so uOpacity=1.0 here is correct.
  FragColor=vec4(col,uOpacity);
}
