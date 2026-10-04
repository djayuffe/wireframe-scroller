#version 410 core
in vec2 uv;
out vec4 FragColor;
uniform float uTime;
uniform float uBeatPhase;
uniform vec2 uResolution;
uniform float uMusicLevel;
uniform float uScrollOffset;
uniform float uTextWidth;
uniform float uOpacity;
// 95 glyphs (ASCII 32..126), 5x7 each, packed row-major into a 665-pixel
// texture (a 1x665 GL_R8 GL_TEXTURE_2D — Apple's Metal-based OpenGL driver
// does not support 1D textures, so a 1-row 2D texture is used instead).
// Each pixel = one row, MSB = leftmost column.
uniform sampler2D uFont;

// Sample one pixel of glyph g at (x 0..4, y 0..6).
float fontPix(float g, float x, float y){
  float idx = g*7.0 + y;
  float bit = 4.0 - x;                 // MSB = leftmost column
  // 1x665 texture: u = (idx+0.5)/665, v = 0.5 (the single row).
  float v = texture(uFont, vec2((idx + 0.5)/665.0, 0.5)).r;
  // Extract bit `bit` of byte v: (v >> bit) & 1  ==  floor(v/2^bit) mod 2.
  // (GLSL built-in is `mod`, not C's `fmod` — Apple's stricter compiler rejects fmod.)
  return mod(floor(v / pow(2.0, bit)), 2.0);
}

void main(){
  vec2 px = uv * uResolution;
  float scale = 6.0;                    // screen px per font px (larger, more readable)
  float glyphW = 5.0 * scale;           // 30 px
  float glyphH = 7.0 * scale;           // 42 px
  float spacing = 1.0 * scale;          // 6 px
  float cellW = glyphW + spacing;       // 36 px
  // Bottom-anchored band (like a marquee/news ticker), 24 px above the bottom.
  float margin = 24.0;
  float bandY0 = uResolution.y - glyphH - margin;
  float y = px.y;
  float a = 0.0;
  if(y >= bandY0 && y < bandY0 + glyphH){
    // Scroll left; wrap so the text re-enters from the right. uScrollOffset is
    // a positive in-cycle distance, so SUBTRACT it (text moves left over time).
    float period = uResolution.x + uTextWidth;
    float wrapped = mod(px.x - uScrollOffset, period);
    if(wrapped < uTextWidth){
      float fi = floor(wrapped / cellW);
      float fx = mod(wrapped, cellW);
      float gx = floor(fx / scale);
      float gy = floor((y - bandY0) / scale);
      if(gx >= 0.0 && gx < 5.0 && gy >= 0.0 && gy < 7.0){
        float glyph = floor(fi) - 32.0;   // char - 32 -> 0..94
        if(glyph >= 0.0 && glyph < 95.0){
          a = fontPix(glyph, gx, gy);
        }
      }
    }
  }
  // Beat pulse: sharp flash on the downbeat (uBeatPhase=0), decays to a soft
  // floor. musicLevel adds a sustained glow.
  float beatFlash = pow(1.0 - clamp(uBeatPhase,0.0,1.0), 3.0);   // 1 at downbeat, ->0
  float music = clamp(uMusicLevel, 0.0, 1.0);
  float pulse = 0.55 + 0.85 * beatFlash + 0.45 * music;
  // Color: cyan-white base, shifts to hot pink with music + a beat flash.
  vec3 base = vec3(0.45, 0.90, 1.00);
  vec3 hot  = vec3(1.00, 0.55, 0.95);
  vec3 flash = vec3(1.00, 1.00, 1.10);
  vec3 col = mix(base, hot, music * 0.55 + beatFlash * 0.25);
  col = mix(col, flash, beatFlash * 0.5);
  col *= pulse;
  // Soft vertical fade at the band edges (8 px).
  float edgeFade = smoothstep(0.0, 8.0, y - bandY0) * smoothstep(0.0, 8.0, (bandY0 + glyphH) - y);
  // Subtle left/right screen-edge fade so text doesn't hard-clip at the borders.
  float xFade = smoothstep(0.0, 40.0, px.x) * smoothstep(0.0, 40.0, uResolution.x - px.x);
  float alpha = a * uOpacity * edgeFade * xFade;
  // PREMULTIPLIED output (rgb *= alpha) — the C++ blend is (ONE, ONE_MINUS_SRC_ALPHA).
  FragColor = vec4(col * alpha, alpha);
}
