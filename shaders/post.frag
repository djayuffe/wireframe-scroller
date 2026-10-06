#version 410 core
in vec2 uv;
out vec4 FragColor;
uniform sampler2D uScene;
uniform vec2 uResolution;
uniform float uTime;
uniform float uMusicLevel;
// Tempo in beats per minute. The synthetic beat pulse below is synced to this
// so the FX pulse at, e.g., --bpm 100 actually pulses at 100 BPM (100/60 Hz)
// instead of a hard-coded 3.2 Hz (192 BPM) that only matches 192 BPM.
uniform float uBpm;
// 1.0 when a logo backdrop is in uScene (logo mode): skip the procedural
// background and reduce the scene gain so the logo isn't washed out.
uniform float uHasLogo;
// --no-post: skip the whole FX chain. Just tone-map the captured HDR buffer and
// output it (the 3D + logo are still composited; only the screen-space FX,
// procedural background, and grain are skipped). This is the fast/debug path.
uniform float uBypass;

float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453123);}
float hash21(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453123);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.-2.*f);return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);}
float fbm(vec2 p){float v=0.,a=.5;for(int i=0;i<4;i++){v+=a*noise(p);p=mat2(.8,-.6,.6,.8)*p*2.03+vec2(3.1,1.7);a*=.5;}return v;}
vec3 pal(float t){return .48+.52*cos(6.28318*(vec3(.02,.28,.56)+t));}
// ACES filmic approximation (Narkowicz 2015).
vec3 aces(vec3 x){const float a=2.51,b=.03,c=2.43,d=.59,e=.14;return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.,1.);}

float tunnelLayer(vec2 p,float z,float twist){
  vec2 q=p*(1.0+z*.42);
  float a=atan(q.y,q.x)+twist+z*.21;
  float r=length(q);
  float ribs=pow(.5+.5*sin(a*18.0+z*7.0),18.0);
  float rings=pow(.5+.5*sin(r*18.0-z*5.5),22.0);
  return (ribs*.55+rings*.45)*smoothstep(1.95,.18,r);
}

void main(){
  // --no-post fast path: sample the captured frame, ACES tone-map it (the buffer
  // is HDR float, so it needs SOME tone map to be visible), output. Skips every
  // FX (no UV warps, no procedural background, no grain, no bloom taps).
  if(uBypass>.5){
    vec3 c=texture(uScene,uv).rgb;
    FragColor=vec4(pow(aces(c*1.25),vec3(.4545)),1.0);
    return;
  }
  vec2 px=1.0/max(uResolution,vec2(1));
  vec2 p=(uv*2.-1.)*vec2(uResolution.x/max(uResolution.y,1.),1.);
  // Synthetic beat pulse, synced to the real tempo (uBpm beats/min). The old
  // hard-coded uTime*3.2 (192 BPM) only matched 192 BPM; at --bpm 132 the FX
  // pulsed ~45% too fast. beatHz = bpm/60, and a 14th-power sine gives the same
  // sharp downbeat spike shape the original had.
  float beatHz=uBpm/60.0;
  float beat=pow(.5+.5*sin(6.2831853*uTime*beatHz),14.0);
  float mlev=uMusicLevel;
  // 1.0 when a logo backdrop is in uScene (logo mode): the logo is a real
  // 0-1 picture, so screen-space UV warps (lensing/shockwave/barrel/CA) would
  // smear it into an unrecognizable smudge. Compute this BEFORE the warps so
  // they can be gated off.
  float hasLogo=step(.5,uHasLogo);

  // --- Screen-space UV warps (applied to the scene sample) -------------------
  // These displace the sampling UV. They look great on a wireframe-on-black
  // but destroy a photographic logo, so every warp is multiplied by
  // (1.0-hasLogo) — a no-op in logo mode.
  vec2 suv=uv;
   // 1) Gravitational lensing: true 1/r^2 central magnification.
   // Clamp the displaced UV to [0,1] — near the center the magnification can
   // push it well outside the buffer, which would sample clamped black borders.
   { vec2 cp=suv-.5; float rr=max(length(cp),.035);
     float lens=(.35+.65*beat)*(1.0-hasLogo);
     suv=clamp(.5+cp*(1.0+lens*.035/(rr*rr)),0.0,1.0); }
  // 2) Refractive shockwave ring: a Gaussian ring that displaces the image.
  { float shock=fract(uTime*.145);
    float r=length(suv-.5);
    float ring=exp(-pow((r-shock*.72)/.025,2.0));
    suv=.5+(suv-.5)*(1.0-ring*.09*(.5+mlev)*(1.0-hasLogo)); }
  // 3) Barrel/lens breathing (pulse-driven).
  { vec2 bp=suv-.5; float r2=dot(bp,bp);
    suv=.5+bp*(1.0+(.02+.05*beat)*r2*(1.0-hasLogo)); }

  // 4) Chromatic aberration (sample R/B at offset UVs, driven by pulse).
  vec2 ca=(suv-.5)*(.002+.006*beat)*(1.0-hasLogo);
  vec3 scene=vec3(texture(uScene,suv+ca).r,texture(uScene,suv).g,texture(uScene,suv-ca).b);

  // bloom (multi-tap)
  vec3 bloom=vec3(0);
  bloom+=texture(uScene,suv+vec2( px.x*1.5,0)).rgb;
  bloom+=texture(uScene,suv+vec2(-px.x*1.5,0)).rgb;
  bloom+=texture(uScene,suv+vec2(0, px.y*1.5)).rgb;
  bloom+=texture(uScene,suv+vec2(0,-px.y*1.5)).rgb;
  bloom+=texture(uScene,suv+vec2( px.x*3.5, px.y*2.5)).rgb;
  bloom+=texture(uScene,suv+vec2(-px.x*3.5,-px.y*2.5)).rgb;
  bloom/=6.;

  float n=fbm(p*1.7+uTime*.035);
  float n2=fbm(p*3.1+vec2(uTime*.05,-uTime*.025));
  float star=step(.996,hash(floor((p+2.)*vec2(120.,70.)+floor(uTime*.25))));
  // Ambient dust: finer, slower, more numerous drifting motes.
  vec2 dp=floor(p*vec2(46.,26.)+uTime*vec2(.35,.18));
  float dh=hash(dp);
  vec2 df=fract(p*vec2(46.,26.)+uTime*vec2(.35,.18));
  float dust=step(.86,dh)*smoothstep(.16,.0,length(df-.5))*(.3+.5*uMusicLevel);
  float neb=pow(max(0.,n),3.)*(.10+.25*uMusicLevel);
  float grid=pow(.5+.5*sin((p.x+p.y*.35)*18.+uTime*.55),18.)*(.018+.06*uMusicLevel);
  float matrixX=pow(.5+.5*sin(p.x*32.+sin(p.y*3.2+uTime*.55)*3.0+uTime*.9),28.);
  float matrixY=pow(.5+.5*sin(p.y*24.+cos(p.x*2.5-uTime*.4)*2.5-uTime*.7),26.);
  float aurora=pow(max(0.,sin(p.y*4.5+n2*5.0+uTime*.72)),3.)*smoothstep(-1.15,.45,p.y);
  float tunnel=0.;
  for(int i=0;i<4;i++){
    float z=fract(float(i)*.25+uTime*.055+uMusicLevel*.035);
    tunnel+=tunnelLayer(p,z,uTime*.12+float(i)*1.7)*(1.0-z)*.25;
  }
  float glimmer=pow(max(0.,sin((p.x*23.-p.y*17.)+uTime*(2.2+uMusicLevel*3.))),28.)*
                smoothstep(.15,1.6,length(p))*(.03+.13*uMusicLevel);
  vec3 bg=vec3(.0015,.003,.013);
  bg+=pal(n*.32+uTime*.015)*neb;
  bg+=pal(p.y*.08+uTime*.035+n2*.15)*aurora*(.045+.16*uMusicLevel);
  bg+=vec3(.35,.55,1.2)*(star*.22+grid);
  bg+=vec3(.7,.8,1.0)*dust;
  bg+=pal(uTime*.04+n2*.18+length(p)*.045)*tunnel*(.08+.22*uMusicLevel);
  bg+=pal(uTime*.06+p.x*.025)*(matrixX+matrixY)*(.018+.075*uMusicLevel);
  bg+=pal(uTime*.05+.3)*glimmer;

  // In logo mode the scene already contains the logo backdrop, so skip the
  // procedural background (it would wash out the picture) and use a lower
  // scene gain (the logo is 0-1, not a bright wireframe on black). hasLogo is
  // already computed above (before the warps).
  vec3 hdr=(1.0-hasLogo)*bg + scene*mix(1.35,1.0,hasLogo) + bloom*(.32+uMusicLevel*.70);
  hdr+=(1.0-hasLogo)*pal(length(p)*.08+uTime*.02)*pow(max(scene.r,max(scene.g,scene.b)),2.2)*(.45+uMusicLevel);

  // --- Uber-compositor signature FX ------------------------------------------
  // 5) SDF nested-triangle energy sculpture (screen space, pulsing).
  { vec2 sp=suv-.5; float aa=atan(sp.y,sp.x);
    float tri=cos(floor(.5+aa/2.0943951)*2.0943951-aa)*length(sp);
    float sdf=exp(-abs(tri-.20-.025*sin(uTime*2.))*120.0);
    hdr+=vec3(1.,.015,.002)*sdf*(.10+.4*beat); }
  // 6) Holographic spectral interference (subtle RGB sin stripes).
  { float holo=.5+.5*sin(uTime*.3);
    hdr+=holo*.04*vec3(sin(uv.x*90.+uTime*4.)*.5+.5,sin(uv.y*90.+uTime*4.+2.1)*.5+.5,sin((uv.x+uv.y)*90.+uTime*4.+4.2)*.5+.5); }
  // 7) Glitch slices (hash-driven horizontal displacement, beat-gated).
  // Gated off in logo mode: re-sampling uScene would re-inject the logo's
  // premultiplied RGB as an uncontrolled horizontal double-exposure.
  { float slice=step(.97,hash21(vec2(floor(uv.y*90.),floor(uTime*12.))));
    hdr+=slice*beat*(1.0-hasLogo)*texture(uScene,vec2(fract(uv.x+.025*sin(uTime*17.)),uv.y)).rgb*.4; }

  // 8) Scanlines / interlace (subtle, always on).
  hdr*=.985+.015*sin(gl_FragCoord.y*3.14159);

  // vignette (kept, slightly stronger)
  float vign=1.-smoothstep(.55,1.85,length(p*vec2(.82,1.)));
  hdr*=max(.30,vign);
  // Pixel-shader soft shadow: a darkened ellipse "cast" below the wireframe,
  // breathing with the scene scale and beat. Centered slightly low. Gated off
  // in logo mode — it would darken the center of the logo picture.
  float shScale=.62+.10*sin(uTime*.41)+.06*uMusicLevel;
  vec2 sc=(p-vec2(0.,-.55))/vec2(shScale,shScale*.5);
  float shadow=exp(-dot(sc,sc))* .38 * (1.0-hasLogo);
  hdr*= (1.0-shadow);
  // 9) Film grain (per-pixel, 60 fps time-quantized).
  hdr+=((hash(gl_FragCoord.xy+floor(uTime*60.))-.5)*.028);
  // 10) ACES tonemap + gentle contrast + gamma.
  // The logo card is already a finished 0..1 picture: expose it lower and skip the mid-tone lift
  // (pow < 1 brightens), or its silver letters burn out to white under the additive wires and bloom.
  hdr=aces(hdr*(.92+.38*mlev)*mix(1.0,.74,hasLogo));
  hdr=(hdr-.5)*1.075+.5;
  hdr=pow(max(hdr,0.),vec3(mix(.86,1.0,hasLogo)));
  FragColor=vec4(hdr,1);
}
