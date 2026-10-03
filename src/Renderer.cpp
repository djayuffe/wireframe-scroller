#include "Renderer.hpp"
#include "TextScroller.hpp"
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
bool Renderer::init(GLFWwindow*,const std::string&dir){std::string vs=readText(dir+"/wire.vert"),fs=readText(dir+"/wire.frag");if(vs.empty()||fs.empty()){error_="cannot read shaders from "+dir;return false;}program_=linkProgram(vs,fs,error_);if(!program_)return false;uMVP_=glGetUniformLocation(program_,"uMVP");uTime_=glGetUniformLocation(program_,"uTime");uColor_=glGetUniformLocation(program_,"uColor");uMusicLevel_=glGetUniformLocation(program_,"uMusicLevel");glGenVertexArrays(1,&vao_);glGenBuffers(1,&vbo_);glGenBuffers(1,&ebo_);glGenVertexArrays(1,&postVao_);if(!initPost(dir))return false;if(!initScroller(dir)){error_="scroller shader failed (non-fatal)";return false;}glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);return true;}
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
void Renderer::shutdown(){if(hdrFbo_)glDeleteFramebuffers(1,&hdrFbo_);if(hdrTex_)glDeleteTextures(1,&hdrTex_);if(depthRbo_)glDeleteRenderbuffers(1,&depthRbo_);if(postProgram_)glDeleteProgram(postProgram_);if(postVao_)glDeleteVertexArrays(1,&postVao_);if(scrollerProgram_)glDeleteProgram(scrollerProgram_);if(scrollerVao_)glDeleteVertexArrays(1,&scrollerVao_);if(program_)glDeleteProgram(program_);if(ebo_)glDeleteBuffers(1,&ebo_);if(vbo_)glDeleteBuffers(1,&vbo_);if(vao_)glDeleteVertexArrays(1,&vao_);program_=postProgram_=scrollerProgram_=vao_=postVao_=scrollerVao_=vbo_=ebo_=hdrFbo_=hdrTex_=depthRbo_=0;}
