#include "TextScroller.hpp"
#include <cmath>
#include <vector>
namespace textfont {
// 5x7 font, rows top->bottom, bit 4 (0x10) = leftmost column.
static const Glyph G[95] = {
/*space*/ {{0,0,0,0,0,0,0}},
/*!*/     {{4,4,4,4,0,4,0}},
/*"*/     {{22,22,0,0,0,0,0}},
/*#*/     {{10,10,62,10,62,10,10}},
/*$*/     {{4,62,68,34,10,62,4}},
/*%*/     {{49,50,4,2,1,49,48}},
/*&*/     {{14,41,41,14,49,24,26}},
/*'*/     {{4,4,2,0,0,0,0}},
/*(*/     {{2,4,8,8,8,4,2}},
/*)*/     {{8,4,2,2,2,4,8}},
/**/      {{0,32,62,3,62,32,0}},
/*+*/     {{0,4,4,62,4,4,0}},
/*,*/     {{0,0,0,0,8,4,8}},
/*-*/     {{0,0,0,62,0,0,0}},
/*./*/    {{0,0,0,0,0,4,4}},
/*0*/     {{14,17,19,21,25,17,14}},
/*1*/     {{4,12,4,4,4,4,62}},
/*2*/     {{14,17,1,2,4,8,62}},
/*3*/     {{62,1,2,1,1,17,14}},
/*4*/     {{2,6,14,26,62,2,2}},
/*5*/     {{62,32,34,1,1,17,14}},
/*6*/     {{6,8,32,34,37,17,14}},
/*7*/     {{62,1,2,4,8,8,8}},
/*8*/     {{14,17,17,14,17,17,14}},
/*9*/     {{14,17,17,15,1,2,12}},
/*:*/     {{0,0,12,0,0,12,0}},
/*;*/     {{0,0,12,0,8,4,8}},
/*<*/     {{2,4,8,16,8,4,2}},
/*=*/     {{0,0,62,0,62,0,0}},
/*>*/     {{8,4,2,1,2,4,8}},
/*?*/     {{14,17,1,2,4,0,4}},
/*@*/     {{14,17,31,37,37,14,15}},
/*A*/     {{14,17,17,17,62,17,17}},
/*B*/     {{60,33,33,60,33,33,60}},
/*C*/     {{14,17,32,32,32,17,14}},
/*D*/     {{60,33,33,33,33,33,60}},
/*E*/     {{62,32,32,34,32,32,62}},
/*F*/     {{62,32,32,34,32,32,32}},
/*G*/     {{14,17,32,39,33,17,15}},
/*H*/     {{17,17,17,62,17,17,17}},
/*I*/     {{14,4,4,4,4,4,14}},
/*J*/     {{29,8,8,8,8,18,12}},
/*K*/     {{17,18,20,24,20,18,17}},
/*L*/     {{32,32,32,32,32,32,62}},
/*M*/     {{17,31,27,21,17,17,17}},
/*N*/     {{17,25,21,19,17,17,17}},
/*O*/     {{14,17,17,17,17,17,14}},
/*P*/     {{62,33,33,62,32,32,32}},
/*Q*/     {{14,17,17,17,21,18,13}},
/*R*/     {{62,33,33,62,20,18,17}},
/*S*/     {{15,33,32,16,1,17,14}},
/*T*/     {{62,4,4,4,4,4,4}},
/*U*/     {{17,17,17,17,17,17,14}},
/*V*/     {{17,17,17,17,10,4,4}},
/*W*/     {{17,17,17,21,27,31,17}},
/*X*/     {{17,17,10,4,10,17,17}},
/*Y*/     {{17,17,10,4,4,4,4}},
/*Z*/     {{62,1,2,4,8,16,62}},
/*[*/     {{14,16,16,16,16,16,14}},
/*\*/     {{32,16,8,4,2,1,0}},
/*]*/     {{14,2,2,2,2,2,14}},
/*^*/     {{4,10,17,0,0,0,0}},
/*_*/     {{0,0,0,0,0,0,62}},
/*`*/     {{8,4,2,0,0,0,0}},
/*a*/     {{0,0,14,1,15,13,14}},
/*b*/     {{16,16,22,33,33,33,62}},
/*c*/     {{0,0,14,16,16,16,14}},
/*d*/     {{1,1,13,33,33,33,63}},
/*e*/     {{0,0,14,33,33,16,14}},
/*f*/     {{2,10,10,28,10,10,10}},
/*g*/     {{0,0,15,13,15,1,14}},
/*h*/     {{16,16,22,33,33,33,33}},
/*i*/     {{4,0,8,4,4,4,14}},
/*j*/     {{2,0,2,2,2,22,18}},
/*k*/     {{16,16,16,20,24,20,18}},
/*l*/     {{12,4,4,4,4,4,14}},
/*m*/     {{0,0,30,27,27,27,27}},
/*n*/     {{0,0,22,33,33,33,33}},
/*o*/     {{0,0,14,17,17,17,14}},
/*p*/     {{0,0,62,33,33,62,32}},
/*q*/     {{0,0,63,33,33,63,1}},
/*r*/     {{0,0,22,33,16,16,16}},
/*s*/     {{0,0,15,16,14,1,12}},
/*t*/     {{8,8,28,8,8,10,2}},
/*u*/     {{0,0,13,13,13,13,31}},
/*v*/     {{0,0,17,17,17,10,4}},
/*w*/     {{0,0,17,17,27,27,10}},
/*x*/     {{0,0,17,10,4,10,17}},
/*y*/     {{0,0,13,13,31,1,14}},
/*z*/     {{0,0,31,2,4,8,31}},
/*{*/     {{3,6,6,32,6,6,3}},
/*|*/     {{4,4,4,4,4,4,4}},
/*}*/     {{12,12,12,2,12,12,12}},
/*~*/     {{0,0,8,16,32,0,0}},
};
const Glyph* glyphs() { return G; }
int width() { return 5; }
int height() { return 7; }
int glyphCount() { return 95; }
int measure(const std::string& s, int spacing) {
  if (s.empty()) return 0;
  return (int)s.size() * (5 + spacing) - spacing;
}
// Pack all glyphs into 95*7 = 665 bytes, row-major (glyph 0 row 0 first).
// Each byte = one row, MSB = leftmost pixel.
std::vector<uint8_t> pack() {
  std::vector<uint8_t> out;
  out.reserve(95 * 7);
  for (int g = 0; g < 95; ++g)
    for (int r = 0; r < 7; ++r)
      out.push_back(G[g].rows[r]);
  return out;
}
}
float scrollOffset(double seconds, float beatPhase, float windowWidthPx,
                    int textWidthPx, float musicLevel) {
  // The scroll speed is CONSTANT (90 px/s) — deliberately NOT eased by beat or
  // music level. The old code did speed = 90*(0.7+0.3*(1-beatPhase)) +
  // music*40 and returned fmod(seconds*speed, wrap). Because `speed` changes
  // every frame (beatPhase/musicLevel are live), seconds*speed teleported the
  // marquee forward/backward each frame instead of advancing smoothly — a
  // visible stutter/jump. A constant speed makes the offset a continuous,
  // monotonic function of time (fmod of a linear distance). beatPhase and
  // musicLevel are kept in the signature for ABI compatibility but unused.
  (void)beatPhase; (void)musicLevel;
  float speed = 90.f;
  // Use double for the long-running distance/wrap: (float)seconds loses
  // sub-pixel precision after a few minutes, causing scroll jitter. Double
  // keeps the in-cycle remainder stable for hours.
  double wrap = (double)windowWidthPx + (double)textWidthPx;
  if (wrap <= 0.0) return 0.f;
  double dist = seconds * (double)speed;
  // In-cycle remainder [0, wrap): the text scrolls left continuously,
  // and re-enters from the right once per wrap.
  return (float)std::fmod(dist, wrap);
}
