#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <mach-o/dyld.h>   // _NSGetExecutablePath
#elif defined(_WIN32)
#include <windows.h>        // GetModuleFileNameA
#else
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#include "Renderer.hpp"
#include "Scene.hpp"
#include "WindowSpec.hpp"
#include "Timeline.hpp"
#include "Audio.hpp"
#include "Effects.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <filesystem>
#include <string>
#include <tuple>
// Set by the SIGINT/SIGTERM handler; the render loop checks it each frame so
// Ctrl-C / kill produces a clean shutdown (r.shutdown + audio.close + SDL/GLFW
// teardown) rather than a hard kill that leaks GL resources.
static std::atomic<bool> g_shutdown{false};
static void onSignal(int){g_shutdown.store(true);}

// Candidate directories for the UBER_Fullscreen_Logo_Pack, in priority order:
// an explicit --logos override, the CWD, the executable's dir, then each
// ancestor of the exe dir (covers build/ -> repo root). Used at startup and
// again after a GL context loss (to re-load the logo textures).
static std::vector<std::filesystem::path> logoCandidates(const std::filesystem::path& overrideDir,const std::filesystem::path& exeDir){
  std::vector<std::filesystem::path> c;
  if(!overrideDir.empty()) c.push_back(overrideDir);
  c.push_back(std::filesystem::current_path()/"UBER_Fullscreen_Logo_Pack");
  if(!exeDir.empty()){
    c.push_back(exeDir/"UBER_Fullscreen_Logo_Pack");
    auto p=exeDir;
    while(p.has_parent_path()&&p.parent_path()!=p){ p=p.parent_path(); c.push_back(p/"UBER_Fullscreen_Logo_Pack"); }
  }
   return c;
 }

int main(int argc,char**argv){
   double bpm=132.0;std::filesystem::path musicPath,logoOverride;bool noScroller=false,noPost=false;
   bool haveRecipe=false; int recipeArg=0;  // haveRecipe=false -> auto mode
   double quality=1.0;  // render scale: the HDR target is W*quality x H*quality
   int winW=0,winH=0; bool fullscreen=false;  // --window WxH / --fullscreen
   std::string screenshotPath; int maxFrames=0;  // --screenshot PATH / --frames N
  for(int i=1;i<argc;i++){
    std::string arg(argv[i]);
    if(arg=="--bpm"&&i+1<argc){double v=std::atof(argv[++i]);if(!std::isfinite(v)){std::fprintf(stderr,"--bpm: not a finite number\n");return 2;}bpm=std::max(1.0,v);}
    else if(arg=="--music"&&i+1<argc)musicPath=argv[++i];
    else if(arg=="--no-scroller")noScroller=true;
    else if(arg=="--no-post")noPost=true;
    else if(arg=="--quality"&&i+1<argc){double v=std::atof(argv[++i]);if(!std::isfinite(v)){std::fprintf(stderr,"--quality: not a finite number\n");return 2;}quality=std::clamp(v,0.25,1.0);}
    else if(arg=="--logos"&&i+1<argc)logoOverride=argv[++i];
    else if(arg=="--window"&&i+1<argc){
       std::string werr; int ww,wh;
       if(!parseWindowSpec(argv[++i],ww,wh,werr)){std::fprintf(stderr,"--window: %s\n",werr.c_str());return 2;}
       winW=ww;winH=wh;
      }
    else if(arg=="--fullscreen")fullscreen=true;
    else if(arg=="--recipe"&&i+1<argc){recipeArg=std::atoi(argv[++i]);haveRecipe=true;}
    else if(arg=="--version"){std::fprintf(stdout,"Impossible Wireframe v4.32\n");return 0;}
    else if(arg=="--screenshot"&&i+1<argc){screenshotPath=argv[++i];}
    else if(arg=="--frames"&&i+1<argc){maxFrames=std::max(0,std::atoi(argv[++i]));}
    else if(arg=="--help"||arg=="-h"){
      std::fprintf(stdout,
        "Impossible Wireframe v4.32\n"
        "Usage: impossible_wireframe [options]\n"
        "  --bpm N         Tempo (default 132)\n"
        "  --music PATH    Audio module/wav to play (default: assets/music if present)\n"
        "  --no-scroller   Start with the text marquee off (toggle with T)\n"
        "  --no-post       Skip the screen-space FX post pass (faster; debugging)\n"
        "  --quality F     Render the HDR target at F of screen resolution (0.25-1.0,\n"
        "                  default 1.0; lower = faster on weak GPUs)\n"
        "  --logos DIR     Directory of UBER_*_1920x1080.jpg logo cards\n"
        "  --recipe N      Start in recipe mode with curated recipe N (0-23)\n"
        "  --window WxH    Initial window size, e.g. --window 1920x1080\n"
        "                  (default 1440x900; 320x200 .. 3840x2160)\n"
        "  --fullscreen    Start full-screen on the primary monitor (uncapped\n"
        "                  refresh; overrides --window)\n"
        "  --screenshot P  Save one PNG frame to P (then continue running)\n"
        "  --frames N      Exit after N frames (0 = run forever; for automated\n"
        "                  verification, e.g. --frames 30 --screenshot out.png)\n"
        "  --version       Print version and exit\n"
        "  -h, --help      Show this help\n"
        "Keys: Left/Right cycle scenes, Space back to auto, R recipe mode,\n"
        "      Up/Down cycle recipe, P re-roll mutation, T toggle scroller, Esc quit\n");
      return 0;}
    }
  // Validate --recipe: an out-of-range index would otherwise silently wrap
  // (recipe(N % count)) and the user would get a different recipe than asked.
  if(haveRecipe){ int n=recipeCount(); if(recipeArg<0||recipeArg>=n){ std::fprintf(stderr,"--recipe N must be in [0,%d] (got %d)\n",n-1,recipeArg); return 2; } }
  // Clean shutdown on Ctrl-C / kill. The handler only sets an atomic flag
  // (async-signal-safe); the render loop checks it and tears down GL/SDL.
  std::signal(SIGINT,onSignal);
  std::signal(SIGTERM,onSignal);
  if(musicPath.empty()){
   std::filesystem::path defaultMusic="assets/music/drozerix_-_silicon_dancer.mod";
   if(std::filesystem::exists(defaultMusic)) musicPath=defaultMusic;
  }
 if(!glfwInit()){std::fprintf(stderr,"GLFW initialization failed\n");return 1;}glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
 glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif
  // --window WxH and --fullscreen. For fullscreen, grab the primary monitor's
  // native mode and use it at full size (the most portable "fullscreen" across
  // macOS/Linux/Windows — true exclusive fullscreen needs per-OS mode-setting,
  // which GLFW abstracts via glfwSetWindowMonitor). If BOTH --window and
  // --fullscreen are given, fullscreen wins (and --window is ignored).
  GLFWmonitor* mon=fullscreen?glfwGetPrimaryMonitor():nullptr;
  const GLFWvidmode* vm=mon?glfwGetVideoMode(mon):nullptr;
  int cw,ch;
  if(fullscreen){
    cw=vm?vm->width:1920; ch=vm?vm->height:1080;
  } else {
    cw=winW>0?winW:1440; ch=winH>0?winH:900;
  }
  GLFWwindow*w=glfwCreateWindow(cw,ch,"Impossible Wireframe v4.32",mon,nullptr);
  if(!w){std::fprintf(stderr,"OpenGL 4.1 context creation failed\n");glfwTerminate();return 2;}
  if(fullscreen)glfwSetWindowMonitor(w,mon,0,0,cw,ch,vm?vm->refreshRate:0);
  glfwMakeContextCurrent(w);glfwSwapInterval(fullscreen?0:1);  // uncapped in fullscreen
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
      #elif defined(_WIN32)
      { char buf[4096]; DWORD n=GetModuleFileNameA(nullptr,buf,sizeof buf);
        if(n>0&&n<sizeof buf){ std::error_code ec; exe=std::filesystem::canonical(buf,ec); } }
      #endif
    if(!exe.empty()&&exe.has_parent_path()){
     exeDir=exe.parent_path();
     auto cand=exeDir/"shaders";
     if(std::filesystem::exists(cand/"wire.vert")) shaderDir=cand.string();
    }
   }
      Renderer r;if(!r.init(w,shaderDir)){std::fprintf(stderr,"Renderer init failed: %s\n",r.error().c_str());audio.close();glfwDestroyWindow(w);glfwTerminate();return 3;}
      r.setRenderScale(float(quality));
      r.setPostEnabled(!noPost);
    { // Load the UBER fullscreen logo pack as the animated background.
       auto candidates=logoCandidates(logoOverride,exeDir);
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
         while(!glfwWindowShouldClose(w) && !g_shutdown.load()){
    // Note: GLFW offers no portable "context lost" query. This block used to test window attribute
    // 0x00020001 as GLFW_CONTEXT_LOST, but that value is GLFW_FOCUSED, so every frame of a focused
    // window was treated as a GPU reset: all GL objects were rebuilt and the frame skipped, which made
    // the demo crawl. Real robustness (GL_ARB_robustness / glGetGraphicsResetStatus) is not available
    // in macOS's OpenGL 4.1, so there is deliberately no reset handling here.
    // Iconified (minimized) windows have no visible framebuffer; rendering is
    // wasted work and on some drivers a no-op that can spam errors. Skip the
    // frame and wait for the window to be restored.
    if(glfwGetWindowAttrib(w,GLFW_ICONIFIED)){
      glfwWaitEventsTimeout(.05);
      continue;
    }
    double now=glfwGetTime();auto music=audio.state();double showSeconds=music.active?music.seconds:now;auto sync=timeline.sample(showSeconds);sync.pulse=std::max(sync.pulse,music.level);int autoScene=int(sync.barIndex/2)%scenes.count();
   if(glfwGetKey(w,GLFW_KEY_T)==GLFW_PRESS){bool want=!scrollerOn;scrollerOn=want;}
   bool L=glfwGetKey(w,GLFW_KEY_LEFT)==GLFW_PRESS,R=glfwGetKey(w,GLFW_KEY_RIGHT)==GLFW_PRESS;
   // Normalize the current scene (auto -> autoScene) before applying the
   // delta so LEFT on scene 0 wraps to the last scene (and RIGHT on the last
   // wraps to 0) instead of leaving manual at -1.
   int curScene=manual<0?autoScene:((manual%scenes.count()+scenes.count())%scenes.count());
   if(L&&!prevL)manual=(curScene-1+scenes.count())%scenes.count();
   if(R&&!prevR)manual=(curScene+1)%scenes.count();
   prevL=L;prevR=R;
   if(glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS)manual=-1;
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
      // The secondary mesh (scene+1) is only used by bridge effects
      // (Nearest Bridge / Morph). Compute it only when the recipe actually
      // references it — otherwise a per-frame 600-cell projection is wasted.
      const Mesh3* secondary=nullptr;
      for(int k=0;k<rc.stageCount;k++)
        if(rc.stages[k].useSecondary){ secondary=m.v.empty()?nullptr:&scenes.mesh((scene+1)%scenes.count(),showSeconds,seed); break; }
      applyRecipe(m,rc,ec,secondary);
    }
    // geo::stats() scans every vertex + edge to compute the radius; on a dense
    // scene (the 600-cell projection is ~1200 edges, a subdivided recipe can be
    // 50k+) it was called TWICE per frame (once for the upload-signature, once
    // for rad). Compute it once and reuse .radius.
    auto st=geo::stats(m);auto sig=std::make_tuple(m.v.size(),m.e.size(),st.radius);if(scene!=lastUploadScene||&m!=lastMesh||sig!=lastMeshSig){if(!r.upload(m)){std::fprintf(stderr,"Mesh rejected in scene %d: %s\n",scene,r.error().c_str());break;}lastUploadScene=scene;lastMesh=&m;lastMeshSig=sig;}int W,H;glfwGetFramebufferSize(w,&W,&H);if(W<=0||H<=0){glfwWaitEventsTimeout(.05);continue;}float rad=std::max(.1f,st.radius);float sizeCycle=1.f+.11f*std::sin(float(showSeconds)*.41f+float(scene)*.37f)+.07f*sync.pulse;
      // 1) Wireframe. draw() captures into the HDR FBO (logo backdrop first in
      //    logo mode, wireframe additive on top in both modes).
      if(!r.draw(float(showSeconds),W,H,float(W)/float(H),scene,std::min(1.48f,1.92f/rad)*sizeCycle,1.f+sync.pulse,music.level)){std::fprintf(stderr,"Renderer draw failed: %s\n",r.error().c_str());break;}
      // 2) Traveling objects weaving back and forth through the wireframe
      //    (captured into the same HDR FBO).
      r.drawTravelers(float(showSeconds),W,H,float(W)/float(H),std::min(1.48f,1.92f/rad)*sizeCycle,music.level);
       // 3) Run the screen-space FX post pass over the captured frame
       //    (logo+wire+travelers in logo mode, wire+travelers otherwise).
       //    Pass bpm so the post shader's synthetic beat pulse is tempo-synced.
       //    With --no-post, the pass still runs (to upscale the scaled HDR target
       //    to the screen + tone-map) but skips every FX via the uBypass path.
       r.finishLogoFrame(float(showSeconds),W,H,scene,music.level,float(bpm));
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
    // --screenshot: capture the finished frame (post pass + scroller already in
    // the default framebuffer) to a PNG. Done after drawScroller so the marquee
    // is included. The capture is one-shot; after it the app keeps running
    // unless --frames also terminates it.
    static bool shotTaken=false;
    static int frameCount=0;   // frames rendered so far (also used by --frames below)
    // With --frames N the picture is taken on the Nth frame (so a timed effect can be sampled);
    // without it, on the first frame.
    if(!screenshotPath.empty()&&!shotTaken&&W>0&&H>0&&(maxFrames<=0||frameCount+1>=maxFrames)){
      if(r.screenshot(screenshotPath,W,H)){std::fprintf(stdout,"screenshot: %s (%dx%d) at show time %.2fs\n",screenshotPath.c_str(),W,H,showSeconds);shotTaken=true;}
      else std::fprintf(stderr,"screenshot failed: %s\n",r.error().c_str());
    }
    // --frames N: exit after N rendered frames (for automated verification).
    // Counts frames actually rendered (not skipped-by-iconify ones).
    if(maxFrames>0){ if(++frameCount>=maxFrames){std::fprintf(stdout,"--frames %d reached; exiting\n",maxFrames);break;} }
   glfwSwapBuffers(w);glfwPollEvents();if(glfwGetKey(w,GLFW_KEY_ESCAPE)==GLFW_PRESS)glfwSetWindowShouldClose(w,1);
  }
 r.shutdown();audio.close();
#if defined(IW_HAS_AUDIO)
 SDL_QuitSubSystem(SDL_INIT_AUDIO);
#endif
 glfwDestroyWindow(w);glfwTerminate();return 0;
}
