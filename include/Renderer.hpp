#pragma once
#include "Geometry.hpp"
#include <cstddef>
#include <string>
#include <vector>
struct GLFWwindow;
class Renderer {
public:
 bool init(GLFWwindow* w,const std::string& shaderDir="shaders");
 bool upload(const Mesh3& m);
  bool draw(float time,int width,int height,float aspect,float scale=1.f,float lineWidth=1.f,float musicLevel=0.f);
  // Fullscreen beat-synced text marquee (scene name + provenance).
  bool drawScroller(float time,float beatPhase,int width,int height,const char*text,float musicLevel=0.f);
  // Logo background: load the UBER_*_1920x1080.jpg cards from a directory.
  bool loadLogos(const std::string& dir);
  int logoCount() const { return (int)logos_.size(); }
  // Draw a full-screen logo backdrop behind the wireframe (crossfades on
  // scene change, subtle Ken Burns). Returns false if no logos loaded.
  bool drawBackground(float time,int width,int height,int sceneIndex,float musicLevel=0.f);
  // Draw the traveling objects moving back and forth through the wireframe.
  bool drawTravelers(float time,int width,int height,float aspect,float scale,float musicLevel=0.f);
  void shutdown();
  const std::string& error() const { return error_; }
private:
  bool initPost(const std::string& shaderDir);
  bool resizeHdr(int width,int height);
  bool initScroller(const std::string& shaderDir);
  bool initBackground(const std::string& shaderDir);
  bool initTravelers(const std::string& shaderDir);
  unsigned vao_=0,vbo_=0,ebo_=0,program_=0,postProgram_=0,postVao_=0,hdrFbo_=0,hdrTex_=0,depthRbo_=0,scrollerProgram_=0,scrollerVao_=0,bgProgram_=0,bgVao_=0,bgTexA_=0,bgTexB_=0,travelerProgram_=0,travelerVao_=0,travelerVbo_=0,travelerEbo_=0; int edgeCount_=0,hdrW_=0,hdrH_=0;
  size_t vboCapacity_=0,eboCapacity_=0; int uMVP_=-1,uTime_=-1,uColor_=-1,uMusicLevel_=-1,uPostScene_=-1,uPostTime_=-1,uPostResolution_=-1,uPostMusic_=-1;
  int uScTime_=-1,uScPhase_=-1,uScRes_=-1,uScMusic_=-1,uScOffset_=-1,uScTextWidth_=-1,uScFont_=-1,uScOpacity_=-1;
  unsigned scrollerFontTex_=0;
  int uBgMix_=-1,uBgZoom_=-1,uBgAspect_=-1,uBgTexAspect_=-1,uBgOp_=-1,uBgTime_=-1,uBgMusic_=-1,uBgTexA_=-1,uBgTexB_=-1;
  int uTrMVP_=-1,uTrTime_=-1,uTrColor_=-1;
  bool hasLogos_=false;
  std::string error_;
  struct Logo { unsigned tex=0; int w=0,h=0; };
  std::vector<Logo> logos_;
  int bgCurrent_=-1;           // logo index currently shown in slot A
  int bgTarget_=-1;            // logo index fading in (slot B)
  float bgMix_=0.f;            // 0=A, 1=B
};
