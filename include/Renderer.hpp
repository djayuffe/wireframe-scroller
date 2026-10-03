#pragma once
#include "Geometry.hpp"
#include <cstddef>
#include <string>
struct GLFWwindow;
class Renderer {
public:
 bool init(GLFWwindow* w,const std::string& shaderDir="shaders");
 bool upload(const Mesh3& m);
 bool draw(float time,int width,int height,float aspect,float scale=1.f,float lineWidth=1.f,float musicLevel=0.f);
 void shutdown();
 const std::string& error() const { return error_; }
private:
 bool initPost(const std::string& shaderDir);
 bool resizeHdr(int width,int height);
 unsigned vao_=0,vbo_=0,ebo_=0,program_=0,postProgram_=0,postVao_=0,hdrFbo_=0,hdrTex_=0,depthRbo_=0; int edgeCount_=0,hdrW_=0,hdrH_=0;
 size_t vboCapacity_=0,eboCapacity_=0; int uMVP_=-1,uTime_=-1,uColor_=-1,uMusicLevel_=-1,uPostScene_=-1,uPostTime_=-1,uPostResolution_=-1,uPostMusic_=-1;
 std::string error_;
};
