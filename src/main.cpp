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
#include "Renderer.hpp"
#include "Scene.hpp"
#include "Timeline.hpp"
#include "Audio.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <tuple>
int main(int argc,char**argv){
 double bpm=132.0;std::filesystem::path musicPath;
 for(int i=1;i<argc;i++){
  std::string arg(argv[i]);
  if(arg=="--bpm"&&i+1<argc)bpm=std::max(1.0,std::atof(argv[++i]));
  else if(arg=="--music"&&i+1<argc)musicPath=argv[++i];
 }
 if(musicPath.empty()){
  std::filesystem::path defaultMusic="assets/music/drozerix_-_silicon_dancer.mod";
  if(std::filesystem::exists(defaultMusic)) musicPath=defaultMusic;
 }
 if(!glfwInit()){std::fprintf(stderr,"GLFW initialization failed\n");return 1;}glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
 glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif
 GLFWwindow*w=glfwCreateWindow(1440,900,"Impossible Wireframe v4.25",nullptr,nullptr);if(!w){std::fprintf(stderr,"OpenGL 4.1 context creation failed\n");glfwTerminate();return 2;}glfwMakeContextCurrent(w);glfwSwapInterval(1);
 AudioPlayer audio;
#if defined(IW_HAS_AUDIO)
 if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0) std::fprintf(stderr,"SDL audio init failed: %s\n",SDL_GetError());
#endif
 if(!musicPath.empty()){
  if(audio.open(musicPath)) std::fprintf(stdout,"music: %s\n",musicPath.string().c_str());
 }
  std::string shaderDir="shaders";
  { // B3 fix: resolve shaders relative to the executable, not the CWD
   std::filesystem::path exe;
   #if defined(__APPLE__)
   { char buf[4096]; uaddrsize_t sz=4096;
     if(_NSGetExecutablePath(buf,&sz)==0){ std::error_code ec; exe=std::filesystem::canonical(buf,ec); } }
   #elif defined(__linux__)
   { std::error_code ec; exe=std::filesystem::canonical("/proc/self/exe",ec); }
   #endif
   if(!exe.empty()&&exe.has_parent_path()){
    auto cand=exe.parent_path()/"shaders";
    if(std::filesystem::exists(cand/"wire.vert")) shaderDir=cand.string();
   }
  }
  Renderer r;if(!r.init(w,shaderDir)){std::fprintf(stderr,"Renderer init failed: %s\n",r.error().c_str());audio.close();glfwDestroyWindow(w);glfwTerminate();return 3;}
 Timeline timeline(bpm);SceneSystem scenes;uint64_t seed=0x49574f424a454354ull;int manual=-1,lastScene=-1,lastUploadScene=-1;bool prevL=false,prevR=false;const Mesh3* lastMesh=nullptr;std::tuple<size_t,size_t,float> lastMeshSig{0,0,-1.f};
  bool scrollerOn=true;
  while(!glfwWindowShouldClose(w)){
   double now=glfwGetTime();auto music=audio.state();double showSeconds=music.active?music.seconds:now;auto sync=timeline.sample(showSeconds);sync.pulse=std::max(sync.pulse,music.level);int autoScene=int(sync.barIndex/2)%scenes.count();
   if(glfwGetKey(w,GLFW_KEY_T)==GLFW_PRESS){bool want=!scrollerOn;scrollerOn=want;}
  bool L=glfwGetKey(w,GLFW_KEY_LEFT)==GLFW_PRESS,R=glfwGetKey(w,GLFW_KEY_RIGHT)==GLFW_PRESS;if(L&&!prevL)manual=(manual<0?autoScene:manual)-1;if(R&&!prevR)manual=(manual<0?autoScene:manual)+1;prevL=L;prevR=R;if(manual>=0){manual=(manual%scenes.count()+scenes.count())%scenes.count();}if(glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS)manual=-1;
  int scene=manual<0?autoScene:manual;const Mesh3&m=scenes.mesh(scene,showSeconds,seed);if(scene!=lastScene){auto&si=scenes.info(scene);std::fprintf(stdout,"scene %02d: %.*s [%.*s]\n",scene,int(si.name.size()),si.name.data(),int(si.provenance.size()),si.provenance.data());lastScene=scene;}
  auto st=geo::stats(m);auto sig=std::make_tuple(m.v.size(),m.e.size(),st.radius);if(scene!=lastUploadScene||&m!=lastMesh||sig!=lastMeshSig){if(!r.upload(m)){std::fprintf(stderr,"Mesh rejected in scene %d: %s\n",scene,r.error().c_str());break;}lastUploadScene=scene;lastMesh=&m;lastMeshSig=sig;}int W,H;glfwGetFramebufferSize(w,&W,&H);if(W<=0||H<=0){glfwWaitEventsTimeout(.05);continue;}float rad=std::max(.1f,geo::stats(m).radius);float sizeCycle=1.f+.11f*std::sin(float(showSeconds)*.41f+float(scene)*.37f)+.07f*sync.pulse;if(!r.draw(float(showSeconds),W,H,float(W)/float(H),std::min(1.48f,1.92f/rad)*sizeCycle,1.f+sync.pulse,music.level)){std::fprintf(stderr,"Renderer draw failed: %s\n",r.error().c_str());break;}
   // Fullscreen beat-synced text marquee: scene name + provenance.
   if(scrollerOn){
    auto&si=scenes.info(scene);
    std::string label;
    label.reserve(si.name.size()+1+si.provenance.size()+16);
    label+=si.name;label+=' ';label+=si.provenance;
    // Prefix with scene index for navigation clarity.
    char idx[16];std::snprintf(idx,sizeof(idx),"[%02d] ",scene);
    label.insert(0,idx);
    r.drawScroller(float(showSeconds),sync.beatPhase,W,H,label.c_str(),music.level);
   }
  glfwSwapBuffers(w);glfwPollEvents();if(glfwGetKey(w,GLFW_KEY_ESCAPE)==GLFW_PRESS)glfwSetWindowShouldClose(w,1);
 }
 r.shutdown();audio.close();
#if defined(IW_HAS_AUDIO)
 SDL_QuitSubSystem(SDL_INIT_AUDIO);
#endif
 glfwDestroyWindow(w);glfwTerminate();return 0;
}
