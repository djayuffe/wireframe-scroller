#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <mach-o/dyld.h>   // _NSGetExecutablePath
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
#include "Effects.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <tuple>
int main(int argc,char**argv){
  double bpm=132.0;std::filesystem::path musicPath,logoOverride;bool noScroller=false;
  bool haveRecipe=false; int recipeArg=0;  // haveRecipe=false -> auto mode
  for(int i=1;i<argc;i++){
    std::string arg(argv[i]);
    if(arg=="--bpm"&&i+1<argc)bpm=std::max(1.0,std::atof(argv[++i]));
    else if(arg=="--music"&&i+1<argc)musicPath=argv[++i];
    else if(arg=="--no-scroller")noScroller=true;
    else if(arg=="--logos"&&i+1<argc)logoOverride=argv[++i];
    else if(arg=="--recipe"&&i+1<argc){recipeArg=std::atoi(argv[++i]);haveRecipe=true;}
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
   std::filesystem::path exeDir;
   { // B3 fix: resolve shaders relative to the executable, not the CWD
    std::filesystem::path exe;
      #if defined(__APPLE__)
      { char buf[4096]; uint32_t sz=sizeof(buf);
        if(_NSGetExecutablePath(buf,&sz)==0){ std::error_code ec; exe=std::filesystem::canonical(buf,ec); } }
     #elif defined(__linux__)
     { std::error_code ec; exe=std::filesystem::canonical("/proc/self/exe",ec); }
     #endif
    if(!exe.empty()&&exe.has_parent_path()){
     exeDir=exe.parent_path();
     auto cand=exeDir/"shaders";
     if(std::filesystem::exists(cand/"wire.vert")) shaderDir=cand.string();
    }
   }
     Renderer r;if(!r.init(w,shaderDir)){std::fprintf(stderr,"Renderer init failed: %s\n",r.error().c_str());audio.close();glfwDestroyWindow(w);glfwTerminate();return 3;}
    { // Load the UBER fullscreen logo pack as the animated background.
     // The binary lives in build/, the pack in the project root. Search the
     // exe dir, then walk up to an ancestor that contains the pack (covers
     // build/ -> repo root), then the CWD.
      std::vector<std::filesystem::path> candidates;
      if(!logoOverride.empty()) candidates.push_back(logoOverride);
      auto cwdPack=std::filesystem::current_path()/"UBER_Fullscreen_Logo_Pack";
      candidates.push_back(cwdPack);
      if(!exeDir.empty()) candidates.push_back(exeDir/"UBER_Fullscreen_Logo_Pack");
      { auto p=exeDir; while(p.has_parent_path()&&p.parent_path()!=p){ p=p.parent_path(); candidates.push_back(p/"UBER_Fullscreen_Logo_Pack"); } }
      bool loaded=false;
      for(auto& c:candidates){ if(r.loadLogos(c.string())){ loaded=true; std::fprintf(stdout,"logos: %d cards from %s\n",r.logoCount(),c.string().c_str()); break; } }
      if(!loaded){ std::fprintf(stderr,"warning: no UBER_Fullscreen_Logo_Pack found; tried:\n"); for(auto& c:candidates)std::fprintf(stderr,"  %s\n",c.string().c_str()); std::fprintf(stderr,"  -> running without logo background\n"); }
    }
 Timeline timeline(bpm);SceneSystem scenes;uint64_t seed=0x49574f424a454354ull;int manual=-1,lastScene=-1,lastUploadScene=-1;bool prevL=false,prevR=false;const Mesh3* lastMesh=nullptr;std::tuple<size_t,size_t,float> lastMeshSig{0,0,-1.f};
    bool scrollerOn=!noScroller;
    // Per-scene effect recipe (the lab's 24 curated recipes) with an optional
    // procedural "mutation" rotation. R toggles recipe mode, +/- cycle it.
    bool recipeMode=haveRecipe, prevF=false, prevP=false; int recipeChoice=haveRecipe?recipeArg:0; uint32_t baseSeed=0x50454646ull;
    std::string lastRecipeName;  // recipe/effect name shown in the scroller
   while(!glfwWindowShouldClose(w)){
   double now=glfwGetTime();auto music=audio.state();double showSeconds=music.active?music.seconds:now;auto sync=timeline.sample(showSeconds);sync.pulse=std::max(sync.pulse,music.level);int autoScene=int(sync.barIndex/2)%scenes.count();
   if(glfwGetKey(w,GLFW_KEY_T)==GLFW_PRESS){bool want=!scrollerOn;scrollerOn=want;}
  bool L=glfwGetKey(w,GLFW_KEY_LEFT)==GLFW_PRESS,R=glfwGetKey(w,GLFW_KEY_RIGHT)==GLFW_PRESS;if(L&&!prevL)manual=(manual<0?autoScene:manual)-1;if(R&&!prevR)manual=(manual<0?autoScene:manual)+1;prevL=L;prevR=R;if(manual>=0){manual=(manual%scenes.count()+scenes.count())%scenes.count();}if(glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS)manual=-1;
  int scene=manual<0?autoScene:manual;Mesh3 m=scenes.mesh(scene,showSeconds,seed);if(scene!=lastScene){auto&si=scenes.info(scene);std::fprintf(stdout,"scene %02d: %.*s [%.*s]\n",scene,int(si.name.size()),si.name.data(),int(si.provenance.size()),si.provenance.data());lastScene=scene;}
    // Toggle effect-recipe mode (R). Auto: per-scene curated recipe (rotated
    // by scene index) + a deterministic mutation overlay. Recipe: hold a fixed
    // curated recipe, Up/Down to cycle.
    bool F=glfwGetKey(w,GLFW_KEY_R)==GLFW_PRESS; if(F&&!prevF){recipeMode=!recipeMode; std::fprintf(stdout,"effects: %s\n",recipeMode?"recipe mode (curated, Up/Down to cycle)":"auto (per-scene curated + mutation)");} prevF=F;
    if(recipeMode){ if(glfwGetKey(w,GLFW_KEY_UP)==GLFW_PRESS)recipeChoice=(recipeChoice+1)%recipeCount(); if(glfwGetKey(w,GLFW_KEY_DOWN)==GLFW_PRESS)recipeChoice=(recipeChoice-1+recipeCount())%recipeCount(); }
    // P: re-roll the mutation seed (new procedural variant for the current scene).
    bool P=glfwGetKey(w,GLFW_KEY_P)==GLFW_PRESS; if(P&&!prevP){baseSeed^=0x9e3779b9u; std::fprintf(stdout,"effects: mutation seed re-rolled (0x%08x)\n",baseSeed);} prevP=P;
    // Drive the lab's CPU effect system. In auto mode each scene gets a curated
    // recipe (rotated by scene index for variety) overlaid with a deterministic
    // mutation. In recipe mode a single curated recipe is held. When audio is
    // active, real bass/mid/treble bands drive the audio-reactive effects;
    // otherwise synthetic sine bands are used.
    {
      EffectContext ec; ec.time=float(showSeconds); ec.amount=1.0f+.6f*float(sync.pulse); ec.beat=sync.pulse;
      if(music.active){ ec.bass=music.bass; ec.mid=music.mid; ec.treble=music.treble; }
      else { ec.bass=.5f+.5f*std::sin(float(showSeconds)*2.0f); ec.mid=.5f+.5f*std::sin(float(showSeconds)*3.3f+1.1f); ec.treble=.5f+.5f*std::sin(float(showSeconds)*5.7f+2.3f); }
      ec.seed=baseSeed+uint32_t(scene)*131u;
      const Mesh3* secondary=m.v.empty()?nullptr:&scenes.mesh((scene+1)%scenes.count(),showSeconds,seed);
      EffectRecipe rc;
      lastRecipeName="";  // reset; filled below
      if(recipeMode){ rc=recipe(recipeChoice); }
      else if(haveRecipe){ rc=recipe(recipeArg%recipeCount()); }
      else {
        // Auto: per-scene curated recipe (rotated by scene index) + a mutation
        // overlay (2 extra deterministic stages) for unique per-frame variation.
        // Combine into a SEPARATE buffer — copying curated->curated in place
        // would be a no-op self-copy and the mutation would never appear.
        int baseRid=scene%recipeCount();
        auto base=recipe(baseRid);
        auto mut=mutateRecipe(ec.seed,2);
        rc=EffectRecipe{}; rc.name="Auto";
        int i=0;
        for(int k=0;k<base.stageCount&&i<6;k++)rc.stages[i++]=base.stages[k];
        for(int k=0;k<mut.stageCount&&i<6;k++)rc.stages[i++]=mut.stages[k];
        rc.stageCount=i;
      }
      lastRecipeName=std::string(rc.name);
      applyRecipe(m,rc,ec,secondary);
    }
    auto st=geo::stats(m);auto sig=std::make_tuple(m.v.size(),m.e.size(),st.radius);if(scene!=lastUploadScene||&m!=lastMesh||sig!=lastMeshSig){if(!r.upload(m)){std::fprintf(stderr,"Mesh rejected in scene %d: %s\n",scene,r.error().c_str());break;}lastUploadScene=scene;lastMesh=&m;lastMeshSig=sig;}int W,H;glfwGetFramebufferSize(w,&W,&H);if(W<=0||H<=0){glfwWaitEventsTimeout(.05);continue;}float rad=std::max(.1f,geo::stats(m).radius);float sizeCycle=1.f+.11f*std::sin(float(showSeconds)*.41f+float(scene)*.37f)+.07f*sync.pulse;
      // 1) Wireframe. In logo mode draw() captures the wireframe into the HDR
      //    FBO; in no-logo mode it runs the post/FX pass directly.
      if(!r.draw(float(showSeconds),W,H,float(W)/float(H),scene,std::min(1.48f,1.92f/rad)*sizeCycle,1.f+sync.pulse,music.level)){std::fprintf(stderr,"Renderer draw failed: %s\n",r.error().c_str());break;}
      // 2) Traveling objects weaving back and forth through the wireframe
      //    (captured into the HDR FBO in logo mode).
      r.drawTravelers(float(showSeconds),W,H,float(W)/float(H),std::min(1.48f,1.92f/rad)*sizeCycle,music.level);
      // 3) Logo mode: composite the fullscreen logo into the capture + run the
      //    screen-space FX post pass over logo+wire+travelers.
      r.finishLogoFrame(float(showSeconds),W,H,scene,music.level);
    // Fullscreen beat-synced text marquee: scene name + provenance + effect
    // recipe + BPM. Shown at the bottom of the screen as a news-ticker band.
    if(scrollerOn){
     auto&si=scenes.info(scene);
     char bpmStr[16];std::snprintf(bpmStr,sizeof(bpmStr)," %dBPM",int(bpm+0.5));
     std::string label;
     label.reserve(si.name.size()+si.provenance.size()+lastRecipeName.size()+40);
     // [NN] SceneName  —  provenance  ·  Effect: RecipeName  ·  132BPM
     char idx[16];std::snprintf(idx,sizeof(idx),"[%02d]",scene);
     label+=idx;label+=' ';label+=si.name;
     label+="  -  ";label+=si.provenance;
     if(!lastRecipeName.empty()){label+="  *  ";label+=lastRecipeName;}
     label+=bpmStr;
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
