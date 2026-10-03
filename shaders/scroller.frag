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
// 95 glyphs (ASCII 32..126), 5x7 each, packed row-major into a 665-byte
// GL_R8 1D texture. Each byte = one row, MSB = leftmost pixel.
uniform sampler1D uFont;

// Sample one pixel of glyph g at (x 0..4, y 0..6).
float fontPix(float g, float x, float y){
  float idx = g*7.0 + y;
  float bit = 4.0 - x;                 // MSB = leftmost column
  float v = texture(uFont, (idx + 0.5)/665.0).r;
  // Extract bit `bit` of byte v: (v >> bit) & 1  ==  floor(v/2^bit) mod 2.
  return fmod(floor(v / pow(2.0, bit)), 2.0);
}

void main(){
  vec2 px = uv * uResolution;
  float scale = 4.0;                    // screen px per font px
  float glyphW = 5.0 * scale;
  float glyphH = 7.0 * scale;
  float spacing = 1.0 * scale;
  float cellW = glyphW + spacing;
  float bandY0 = (uResolution.y - glyphH) * 0.5;
  float y = px.y;
  float a = 0.0;
  if(y >= bandY0 && y < bandY0 + glyphH){
    // Scroll left; wrap so the text re-enters from the right.
    float period = uResolution.x + uTextWidth;
    float wrapped = mod(px.x + uScrollOffset, period);
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
  // Brightness: strong on the downbeat, boosted by music.
  float pulse = 0.6 + 0.55 * (1.0 - uBeatPhase);
  float music = clamp(uMusicLevel, 0.0, 1.0);
  vec3 base = vec3(0.45, 0.9, 1.0);
  vec3 hot  = vec3(1.0, 0.6, 0.95);
  vec3 col = mix(base, hot, music * 0.6) * (0.9 + 0.5 * pulse + music * 0.5);
  float edgeFade = smoothstep(0.0, 6.0, y - bandY0) * smoothstep(0.0, 6.0, (bandY0 + glyphH) - y);
  float alpha = a * uOpacity * edgeFade;
  FragColor = vec4(col * alpha, alpha);
}
