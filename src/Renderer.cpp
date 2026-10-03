#include "Renderer.hpp"
#include "TextScroller.hpp"
#include "Image.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>
namespace {std::string readText(const std::string&p){std::ifstream f(p);if(!f)return {};std::ostringstream s;s<<f.rdbuf();return s.str();}
bool shader(GLuint&out,GLenum type,const std::string&s,std::string&err){out=glCreateShader(type);const char*p=s.c_str();glShaderSource(out,1,&p,nullptr);glCompileShader(out);GLint ok=0;glGetShaderiv(out,GL_COMPILE_STATUS,&ok);if(!ok){GLint n=0;glGetShaderiv(out,GL_INFO_LOG_LENGTH,&n);std::string log(std::max(1,n),'\0');glGetShaderInfoLog(out,n,nullptr,log.data());err=log;glDeleteShader(out);out=0;return false;}return true;}
GLuint linkProgram(const std::string&vs,const std::string&fs,std::string&err){GLuint v=0,f=0;if(!shader(v,GL_VERTEX_SHADER,vs,err)||!shader(f,GL_FRAGMENT_SHADER,fs,err)){if(v)glDeleteShader(v);if(f)glDeleteShader(f);return 0;}GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);GLint ok=0;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok){GLint n=0;glGetProgramiv(p,GL_INFO_LOG_LENGTH,&n);err.resize(std::max(1,n));glGetProgramInfoLog(p,n,nullptr,err.data());glDeleteProgram(p);return 0;}return p;}
void perspective(float*m,float fovy,float aspect,float zn,float zf){float f=1/std::tan(fovy*.5f);for(int i=0;i<16;i++)m[i]=0;m[0]=f/aspect;m[5]=f;m[10]=(zf+zn)/(zn-zf);m[11]=-1;m[14]=(2*zf*zn)/(zn-zf);}
void mul(float*o,const float*a,const float*b){float r[16]{};for(int c=0;c<4;c++)for(int rr=0;rr<4;rr++)for(int k=0;k<4;k++)r[c*4+rr]+=a[k*4+rr]*b[c*4+k];std::copy(r,r+16,o);}}
bool Renderer::init(GLFWwindow*,const std::string&dir){std::string vs=readText(dir+"/wire.vert"),fs=readText(dir+"/wire.frag");if(vs.empty()||fs.empty()){error_="cannot read shaders from "+dir;return false;}program_=linkProgram(vs,fs,error_);if(!program_)return false;uMVP_=glGetUniformLocation(program_,"uMVP");uTime_=glGetUniformLocation(program_,"uTime");uColor_=glGetUniformLocation(program_,"uColor");uMusicLevel_=glGetUniformLocation(program_,"uMusicLevel");glGenVertexArrays(1,&vao_);glGenBuffers(1,&vbo_);glGenBuffers(1,&ebo_);glGenVertexArrays(1,&postVao_);if(!initPost(dir))return false;if(!initScroller(dir)){error_="scroller shader failed (non-fatal)";return false;}
  // Non-fatal extras: logo background + traveling objects. A missing shader
  // degrades to "no background"/"no travelers" rather than failing the app.
  initBackground(dir);
  initTravelers(dir);
  glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);return true;}
bool Renderer::initBackground(const std::string&dir){std::string vs=readText(dir+"/bg.vert"),fs=readText(dir+"/bg.frag");if(vs.empty()||fs.empty())return false;bgProgram_=linkProgram(vs,fs,error_);if(!bgProgram_)return false;uBgMix_=glGetUniformLocation(bgProgram_,"uMix");uBgScaleA_=glGetUniformLocation(bgProgram_,"uScaleA");uBgScaleB_=glGetUniformLocation(bgProgram_,"uScaleB");uBgOp_=glGetUniformLocation(bgProgram_,"uOpacity");uBgTime_=glGetUniformLocation(bgProgram_,"uTime");uBgMusic_=glGetUniformLocation(bgProgram_,"uMusicLevel");uBgTexA_=glGetUniformLocation(bgProgram_,"uTexA");uBgTexB_=glGetUniformLocation(bgProgram_,"uTexB");glGenVertexArrays(1,&bgVao_);return true;}
bool Renderer::initTravelers(const std::string&dir){std::string vs=readText(dir+"/traveler.vert"),fs=readText(dir+"/traveler.frag");if(vs.empty()||fs.empty())return false;travelerProgram_=linkProgram(vs,fs,error_);if(!travelerProgram_)return false;uTrMVP_=glGetUniformLocation(travelerProgram_,"uMVP");uTrTime_=glGetUniformLocation(travelerProgram_,"uTime");uTrColor_=glGetUniformLocation(travelerProgram_,"uColor");
  // Build 12 travelers, each a unit octahedron (6 verts, 12 edges), with a
  // per-vertex path attribute.
  const int TR=12;
  std::vector<float> data;
  data.reserve(TR*6*2*2); // TR travelers * 6 verts * (pos3? no: pos3 + path1) -> we interleave as location0=pos3, location1=path (stride 16B)
  // unit octahedron vertices
  float ov[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
  int oe[12][2]={{0,2},{0,3},{0,4},{0,5},{1,2},{1,3},{1,4},{1,5},{2,4},{2,5},{3,4},{3,5}};
  std::vector<uint32_t> idx;
  for(int t=0;t<TR;t++){
    float path=(float)t/(float)(TR-1);
    for(int vi=0;vi<6;vi++){
      data.push_back(ov[vi][0]);data.push_back(ov[vi][1]);data.push_back(ov[vi][2]);data.push_back(path);
    }
  }
  for(int t=0;t<TR;t++){
    for(int ei=0;ei<12;ei++){
      idx.push_back((uint32_t)(t*6+oe[ei][0]));
      idx.push_back((uint32_t)(t*6+oe[ei][1]));
    }
  }
  glGenVertexArrays(1,&travelerVao_);
  glGenBuffers(1,&travelerVbo_);
  glGenBuffers(1,&travelerEbo_);
  glBindVertexArray(travelerVao_);
  glBindBuffer(GL_ARRAY_BUFFER,travelerVbo_);
  glBufferData(GL_ARRAY_BUFFER,data.size()*sizeof(float),data.data(),GL_STATIC_DRAW);
  glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,16,nullptr);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,16,(void*)(12));
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,travelerEbo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,idx.size()*sizeof(uint32_t),idx.data(),GL_STATIC_DRAW);
  glBindVertexArray(0);
  return true;}
bool Renderer::initScroller(const std::string&dir){std::string vs=readText(dir+"/scroller.vert"),fs=readText(dir+"/scroller.frag");if(vs.empty()||fs.empty()){error_="cannot read scroller shaders";return false;}scrollerProgram_=linkProgram(vs,fs,error_);if(!scrollerProgram_)return false;uScTime_=glGetUniformLocation(scrollerProgram_,"uTime");uScPhase_=glGetUniformLocation(scrollerProgram_,"uBeatPhase");uScRes_=glGetUniformLocation(scrollerProgram_,"uResolution");uScMusic_=glGetUniformLocation(scrollerProgram_,"uMusicLevel");uScOffset_=glGetUniformLocation(scrollerProgram_,"uScrollOffset");uScTextWidth_=glGetUniformLocation(scrollerProgram_,"uTextWidth");uScFont_=glGetUniformLocation(scrollerProgram_,"uFont");glGenVertexArrays(1,&scrollerVao_);GLuint tex=0;glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_1D,tex);auto data=textfont::pack();glTexImage1D(GL_TEXTURE_1D,0,GL_R8,(GLsizei)data.size(),0,GL_RED,GL_UNSIGNED_BYTE,data.data());glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glUseProgram(scrollerProgram_);if(uScFont_>=0)glUniform1i(uScFont_,1);glBindTexture(GL_TEXTURE_1D,0);glActiveTexture(GL_TEXTURE0);return true;}
bool Renderer::initPost(const std::string&dir){std::string vs=readText(dir+"/post.vert"),fs=readText(dir+"/post.frag");if(vs.empty()||fs.empty()){error_="cannot read post shaders from "+dir;return false;}postProgram_=linkProgram(vs,fs,error_);if(!postProgram_)return false;uPostScene_=glGetUniformLocation(postProgram_,"uScene");uPostTime_=glGetUniformLocation(postProgram_,"uTime");uPostResolution_=glGetUniformLocation(postProgram_,"uResolution");uPostMusic_=glGetUniformLocation(postProgram_,"uMusicLevel");glUseProgram(postProgram_);if(uPostScene_>=0)glUniform1i(uPostScene_,0);return true;}
bool Renderer::resizeHdr(int width,int height){width=std::max(1,width);height=std::max(1,height);if(width==hdrW_&&height==hdrH_&&hdrFbo_)return true;hdrW_=width;hdrH_=height;if(!hdrFbo_)glGenFramebuffers(1,&hdrFbo_);if(!hdrTex_)glGenTextures(1,&hdrTex_);if(!depthRbo_)glGenRenderbuffers(1,&depthRbo_);glBindTexture(GL_TEXTURE_2D,hdrTex_);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,width,height,0,GL_RGBA,GL_FLOAT,nullptr);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);glBindRenderbuffer(GL_RENDERBUFFER,depthRbo_);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);glBindFramebuffer(GL_FRAMEBUFFER,hdrFbo_);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,hdrTex_,0);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depthRbo_);if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){error_="HDR framebuffer incomplete";glBindFramebuffer(GL_FRAMEBUFFER,0);return false;}glBindFramebuffer(GL_FRAMEBUFFER,0);return true;}
bool Renderer::upload(const Mesh3&m){std::string why;if(!geo::validate(m,&why)){error_=why;return false;}std::vector<uint32_t>ix;ix.reserve(m.e.size()*2);for(auto e:m.e){ix.push_back(e.a);ix.push_back(e.b);}edgeCount_=(int)ix.size();glBindVertexArray(vao_);glBindBuffer(GL_ARRAY_BUFFER,vbo_);size_t vb=m.v.size()*sizeof(V3);if(vb>vboCapacity_){vboCapacity_=std::max(vb,vboCapacity_*2+4096);glBufferData(GL_ARRAY_BUFFER,vboCapacity_,nullptr,GL_DYNAMIC_DRAW);}if(vb)glBufferSubData(GL_ARRAY_BUFFER,0,vb,m.v.data());glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(V3),nullptr);glEnableVertexAttribArray(0);glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo_);size_t eb=ix.size()*sizeof(uint32_t);if(eb>eboCapacity_){eboCapacity_=std::max(eb,eboCapacity_*2+4096);glBufferData(GL_ELEMENT_ARRAY_BUFFER,eboCapacity_,nullptr,GL_DYNAMIC_DRAW);}if(eb)glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,0,eb,ix.data());return true;}
bool Renderer::draw(float t,int width,int height,float aspect,float scale,float lineWidth,float musicLevel){if(!resizeHdr(width,height))return false;float ml=std::clamp(musicLevel,0.f,1.f);
 float fly=std::sin(t*.19f),zoom=std::sin(t*.23f+1.7f),breath=1.f+.075f*std::sin(t*.83f)+.12f*ml;
 float P[16],V[16]={1,0,0,0,0,1,0,0,0,0,1,0,.22f*fly,.14f*std::cos(t*.13f),-4.25f+.52f*zoom-.28f*ml,1};
 float Ry[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},Rx[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},R[16],X[16],M[16];
 float cy=std::cos(t*.17f),sy=std::sin(t*.17f),cx=std::cos(.27f*std::sin(t*.11f)),sx=std::sin(.27f*std::sin(t*.11f));
 Ry[0]=cy*scale*breath;Ry[2]=-sy*scale*breath;Ry[8]=sy*scale*breath;Ry[10]=cy*scale*breath;Ry[5]=scale*breath;
 Rx[5]=cx;Rx[6]=sx;Rx[9]=-sx;Rx[10]=cx;
 perspective(P,(52.f+3.f*std::sin(t*.07f)-2.f*ml)*3.14159265f/180.f,std::max(.05f,aspect),.05f,100.f);mul(R,Ry,Rx);mul(X,V,R);mul(M,P,X);glBindFramebuffer(GL_FRAMEBUFFER,hdrFbo_);glViewport(0,0,width,height);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);glUseProgram(program_);glUniformMatrix4fv(uMVP_,1,GL_FALSE,M);glUniform1f(uTime_,t);if(uMusicLevel_>=0)glUniform1f(uMusicLevel_,ml);glUniform3f(uColor_,1.2f+.75f*ml,1.55f+.35f*ml,2.1f+1.1f*ml);glLineWidth(std::max(1.f,lineWidth+ml*.75f));glBindVertexArray(vao_);glDrawElements(GL_LINES,edgeCount_,GL_UNSIGNED_INT,nullptr);glBindFramebuffer(GL_FRAMEBUFFER,0);glDisable(GL_DEPTH_TEST);glUseProgram(postProgram_);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,hdrTex_);if(uPostTime_>=0)glUniform1f(uPostTime_,t);if(uPostResolution_>=0)glUniform2f(uPostResolution_,float(width),float(height));if(uPostMusic_>=0)glUniform1f(uPostMusic_,ml);glBindVertexArray(postVao_);glDrawArrays(GL_TRIANGLES,0,3);return true;}
bool Renderer::loadLogos(const std::string&dir){
  auto files=findLogos(dir);
  if(files.empty())return false;
  for(auto& f:files){
    Image img;
    if(!Image::loadFromFile(f,img)) continue;
    Logo L;
    glGenTextures(1,&L.tex);
    glBindTexture(GL_TEXTURE_2D,L.tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,img.w,img.h,0,GL_RGBA,GL_UNSIGNED_BYTE,img.rgba.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    L.w=img.w;L.h=img.h;
    logos_.push_back(L);
  }
  glBindTexture(GL_TEXTURE_2D,0);
  if(logos_.empty())return false;
  bgCurrent_=0;bgTarget_=0;bgMix_=0.f;
  return true;
}
bool Renderer::drawBackground(float time,int width,int height,int sceneIndex,float musicLevel){
  if(!bgProgram_||logos_.empty())return false;
  int target=(logos_.size()>1)?(sceneIndex%(int)logos_.size()):0;
  // When the target logo changes, swap it into slot B and start fading.
  if(target!=bgTarget_){
    // Move current A->B so the new target becomes B and we fade A->B.
    // Keep it simple: if A==target already, nothing to do.
    if(target!=bgCurrent_){
      // copy: promote A to B (swap textures), set B=target, fade 0->1.
      bgTexB_=logos_[target].tex;
      bgMix_=0.f;
    } else {
      bgMix_=1.f;
    }
    bgTarget_=target;
  }
  // Advance the crossfade.
  if(bgMix_<1.f) bgMix_=std::min(1.f,bgMix_+0.02f);
  if(bgMix_>=1.f){ // done: A becomes the old B
    bgCurrent_=bgTarget_;
    bgTexA_=logos_[bgCurrent_].tex;
    bgMix_=0.f;
  }
  if(!bgTexA_) bgTexA_=logos_[0].tex;
  if(!bgTexB_) bgTexB_=logos_[0].tex;
  // Subtle Ken Burns: slow zoom oscillation, per-slot phase offset.
  float kbA=1.06f+0.05f*std::sin(time*0.07f);
  float kbB=1.06f+0.05f*std::sin(time*0.07f+3.14f);
  glUseProgram(bgProgram_);
  if(uBgMix_>=0)glUniform1f(uBgMix_,bgMix_);
  if(uBgScaleA_>=0)glUniform1f(uBgScaleA_,kbA);
  if(uBgScaleB_>=0)glUniform1f(uBgScaleB_,kbB);
  if(uBgOp_>=0)glUniform1f(uBgOp_,0.92f);
  if(uBgTime_>=0)glUniform1f(uBgTime_,time);
  if(uBgMusic_>=0)glUniform1f(uBgMusic_,std::clamp(musicLevel,0.f,1.f));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D,bgTexA_);
  if(uBgTexA_>=0)glUniform1i(uBgTexA_,0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D,bgTexB_);
  if(uBgTexB_>=0)glUniform1i(uBgTexB_,1);
  glDisable(GL_DEPTH_TEST);
  glViewport(0,0,width,height);
  glBindVertexArray(bgVao_);
  glDrawArrays(GL_TRIANGLES,0,3);
  glActiveTexture(GL_TEXTURE0);
  return true;
}
bool Renderer::drawTravelers(float time,int width,int height,float aspect,float scale,float musicLevel){
  if(!travelerProgram_)return false;
  (void)width;(void)height;
  // Reuse the same view/projection the wireframe used so travelers sit inside
  // the same 3D space. Reconstruct from the same params as draw().
  float fly=std::sin(time*.19f),zoom=std::sin(time*.23f+1.7f),breath=1.f+.075f*std::sin(time*.83f)+.12f*std::clamp(musicLevel,0.f,1.f);
  float P[16],V[16]={1,0,0,0,0,1,0,0,0,0,1,0,.22f*fly,.14f*std::cos(time*.13f),-4.25f+.52f*zoom-.28f*std::clamp(musicLevel,0.f,1.f),1};
  float Ry[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},Rx[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},R[16],X[16],M[16];
  float cy=std::cos(time*.17f),sy=std::sin(time*.17f),cx=std::cos(.27f*std::sin(time*.11f)),sx=std::sin(.27f*std::sin(time*.11f));
  Ry[0]=cy*scale*breath;Ry[2]=-sy*scale*breath;Ry[8]=sy*scale*breath;Ry[10]=cy*scale*breath;Ry[5]=scale*breath;
  Rx[5]=cx;Rx[6]=sx;Rx[9]=-sx;Rx[10]=cx;
  perspective(P,(52.f+3.f*std::sin(time*.07f))*3.14159265f/180.f,std::max(.05f,aspect),.05f,100.f);
  mul(R,Ry,Rx);mul(X,V,R);mul(M,P,X);
  glEnable(GL_DEPTH_TEST);
  glUseProgram(travelerProgram_);
  if(uTrMVP_>=0)glUniformMatrix4fv(uTrMVP_,1,GL_FALSE,M);
  if(uTrTime_>=0)glUniform1f(uTrTime_,time);
  if(uTrColor_>=0)glUniform3f(uTrColor_,1.0f,0.9f,1.2f);
  glLineWidth(1.4f+std::clamp(musicLevel,0.f,1.f)*1.0f);
  glBindVertexArray(travelerVao_);
  glDrawElements(GL_LINES,144,GL_UNSIGNED_INT,nullptr);
  return true;
}
bool Renderer::drawScroller(float time,float beatPhase,int width,int height,const char*text,float musicLevel){
  if(!scrollerProgram_||!text||!text[0])return false;
  // Measure text in font pixels (scale=4, spacing=1).
  float scale=4.0f;
  float textWidth=(float)textfont::measure(text,1)*scale;
  float offset=scrollOffset(time,beatPhase,(float)width,(int)textWidth,musicLevel);
  glUseProgram(scrollerProgram_);
  if(uScTime_>=0)glUniform1f(uScTime_,time);
  if(uScPhase_>=0)glUniform1f(uScPhase_,beatPhase);
  if(uScRes_>=0)glUniform2f(uScRes_,(float)width,(float)height);
  if(uScMusic_>=0)glUniform1f(uScMusic_,std::clamp(musicLevel,0.f,1.f));
  if(uScOffset_>=0)glUniform1f(uScOffset_,offset);
  if(uScTextWidth_>=0)glUniform1f(uScTextWidth_,textWidth);
  glActiveTexture(GL_TEXTURE1);
  // Re-upload font texture (cheap, 665 bytes).
  static GLuint fontTex=0;
  if(!fontTex)glGenTextures(1,&fontTex);
  glBindTexture(GL_TEXTURE_1D,fontTex);
  auto data=textfont::pack();
  glTexImage1D(GL_TEXTURE_1D,0,GL_R8,(GLsizei)data.size(),0,GL_RED,GL_UNSIGNED_BYTE,data.data());
  glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  if(uScFont_>=0)glUniform1i(uScFont_,1);
  glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);
  glViewport(0,0,width,height);
  glBindVertexArray(scrollerVao_);
  glDrawArrays(GL_TRIANGLES,0,3);
  glDisable(GL_BLEND);
  glActiveTexture(GL_TEXTURE0);
  return true;
}
void Renderer::shutdown(){for(auto&L:logos_)if(L.tex)glDeleteTextures(1,&L.tex);logos_.clear();if(hdrFbo_)glDeleteFramebuffers(1,&hdrFbo_);if(hdrTex_)glDeleteTextures(1,&hdrTex_);if(depthRbo_)glDeleteRenderbuffers(1,&depthRbo_);if(postProgram_)glDeleteProgram(postProgram_);if(postVao_)glDeleteVertexArrays(1,&postVao_);if(scrollerProgram_)glDeleteProgram(scrollerProgram_);if(scrollerVao_)glDeleteVertexArrays(1,&scrollerVao_);if(bgProgram_)glDeleteProgram(bgProgram_);if(bgVao_)glDeleteVertexArrays(1,&bgVao_);if(travelerProgram_)glDeleteProgram(travelerProgram_);if(travelerVao_)glDeleteVertexArrays(1,&travelerVao_);if(travelerVbo_)glDeleteBuffers(1,&travelerVbo_);if(travelerEbo_)glDeleteBuffers(1,&travelerEbo_);if(program_)glDeleteProgram(program_);if(ebo_)glDeleteBuffers(1,&ebo_);if(vbo_)glDeleteBuffers(1,&vbo_);if(vao_)glDeleteVertexArrays(1,&vao_);program_=postProgram_=scrollerProgram_=bgProgram_=travelerProgram_=vao_=postVao_=scrollerVao_=bgVao_=travelerVao_=vbo_=ebo_=travelerVbo_=travelerEbo_=hdrFbo_=hdrTex_=depthRbo_=0;}
