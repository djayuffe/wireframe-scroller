#version 410 core
in vec2 uv;
out vec4 FragColor;
uniform float uTime;
uniform float uBeatPhase;
uniform vec2 uResolution;
uniform float uMusicLevel;
uniform float uScrollOffset;
uniform float uTextWidth;
uniform float uFade;
// 95 glyphs (ASCII 32..126), 5x7 each, packed as 7 bytes per glyph.
// MSB = leftmost pixel. Uploaded as a 1D UNSIGNED_BYTE texture (GL_R8).
uniform sampler1D uFont;

// Sample one pixel of the glyph at (g, x, y): g in 0..94, x 0..4, y 0..6.
float fontPix(float g, float x, float y){
  float idx = g*7.0 + y;
  float bit = 4.0 - x;
  float v = texture(uFont, (idx + 0.5)/665.0).r;
  float b = floor(v / (1.0 + pow(2.0, bit)));
  return fract(b);
}

void main(){
  vec2 px = uv * uResolution;
  // Text band: centered vertically, height ~ 7 * scale.
  float scale = 4.0;                 // pixel size per font pixel
  float glyphW = 5.0 * scale;
  float glyphH = 7.0 * scale;
  float spacing = 1.0 * scale;
  float cellW = glyphW + spacing;
  float bandH = glyphH;
  float bandY0 = (uResolution.y - bandH) * 0.5;
  float y = px.y;
  float a = 0.0;
  if(y >= bandY0 && y < bandY0 + bandH){
    float lx = px.x;
    // Wrap: text scrolls left by uScrollOffset; period = width + textWidth.
    float period = uResolution.x + uTextWidth;
    float total = lx + uScrollOffset;
    float wrapped = mod(total, period);
    // Only draw where wrapped falls within [0, textWidth).
    if(wrapped < uTextWidth){
      float fi = floor(wrapped / cellW);
      float fx = mod(wrapped, cellW) - glyphW * 0.5;
      float gy = (y - bandY0);
      float gx = floor(fx / scale);
      float gyp = floor(gy / scale);
      if(gx >= 0.0 && gx < 5.0 && gyp >= 0.0 && gyp < 7.0){
        float glyph = floor(fi) - 32.0;   // char index 0..94
        if(glyph >= 0.0 && glyph < 95.0){
          a = fontPix(glyph, gx, gyp);
        }
      }
    }
  }
  // Beat pulse: brightest on downbeat, eased.
  float pulse = 0.55 + 0.45 * (1.0 - uBeatPhase);
  float music = clamp(uMusicLevel, 0.0, 1.0);
  vec3 base = vec3(0.35, 0.85, 1.0);
  vec3 hot  = vec3(1.0, 0.55, 0.9);
  vec3 col = mix(base, hot, music * 0.6) * (0.7 + 0.5 * pulse + music * 0.4);
  // Vertical fade at band edges for a soft marquee look.
  float edgeFade = smoothstep(0.0, 8.0, y - bandY0) * smoothstep(0.0, 8.0, (bandY0 + bandH) - y);
  float alpha = a * uFade * edgeFade;
  FragColor = vec4(col * alpha, alpha);
}
