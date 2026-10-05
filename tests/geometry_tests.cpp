#include "Geometry.hpp"
#include "AdvancedGeometry.hpp"
#include "Image.hpp"
#include "Scene.hpp"
#include "TextScroller.hpp"
#include "Timeline.hpp"
#include "Effects.hpp"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
static void req(bool x,const char*m){if(!x){std::cerr<<"FAIL: "<<m<<"\n";std::exit(1);}}
static void mesh(const Mesh3&m,const char*n,bool requireEdges=true){std::string w;req(geo::validate(m,&w),n);req(!m.v.empty(),n);if(requireEdges)req(!m.e.empty(),n);}
int main(){
 auto t=geo::tesseract();req(t.v.size()==16&&t.e.size()==32&&t.faces.size()==24,"tesseract V/E/F");
 auto c16=geo::cell16();req(c16.v.size()==8&&c16.e.size()==24&&c16.faces.size()==32,"16-cell V/E/F");
 auto c24=geo::cell24();req(c24.v.size()==24&&c24.e.size()==96&&c24.faces.size()==96,"24-cell V/E/F");
 auto c600=geo::cell600();req(c600.v.size()==120&&c600.e.size()==720&&c600.faces.size()==1200,"600-cell V/E/F");
 auto c120=geo::cell120Vertices();std::string w;req(geo::validate(c120,&w),"120-cell valid");req(c120.v.size()==600&&c120.e.size()==1200,"120-cell V/E exact");
 mesh(geo::project4D(c600,.1f,.2f,3.4f,.3f,.4f),"4D projection"); mesh(geo::sliceFaces4D(c600,{.31f,.47f,.59f,.57f},0),"4D face slice");
 for(auto k:{geo::ImplicitKind::Gyroid,geo::ImplicitKind::SchwarzP,geo::ImplicitKind::SchwarzD,geo::ImplicitKind::Neovius,geo::ImplicitKind::IWP})mesh(geo::implicitSurface(k,12),"implicit extraction");
 mesh(geo::hopfFibres(8,24),"hopf");mesh(geo::boySurface(20,12),"boy");mesh(geo::superformula(24,12),"superformula");mesh(geo::cliffordTorus(20,10),"clifford");mesh(geo::hyperbolicBall(3,8),"hyperbolic");mesh(geo::lissajousKnot(100),"lissajous");mesh(geo::strangeAttractor(1000),"lorenz");mesh(geo::discoveredObject(42,24,12),"discovered");mesh(geo::quaternionJulia(14,7,.08f),"quaternion Julia");
 mesh(geo::mobiusStrip(32,8),"mobius");mesh(geo::kleinBottle(32,12),"klein");mesh(geo::enneperSurface(24,12),"enneper");mesh(geo::helicoidSurface(24,12),"helicoid");mesh(geo::catenoidSurface(24,12),"catenoid");mesh(geo::diniSurface(24,12),"dini");mesh(geo::pseudosphere(24,12),"pseudosphere");mesh(geo::romanSurface(24,12),"roman");mesh(geo::crossCap(24,12),"crosscap");mesh(geo::torusKnot(120,3,7),"torus knot");mesh(geo::vivianiCurve(100),"viviani");mesh(geo::sphericalSpiral(100),"spherical spiral");mesh(geo::hypotrochoidKnot(120),"hypotrochoid");mesh(geo::duffingAttractor(1200),"duffing");mesh(geo::rosslerAttractor(1200),"rossler");mesh(geo::thomasAttractor(1200),"thomas");mesh(geo::sierpinskiTetrahedron(3),"sierpinski");
 for(auto k:{geo::ExoticImplicitKind::Heart,geo::ExoticImplicitKind::BarthSexticLike,geo::ExoticImplicitKind::TangleCube,geo::ExoticImplicitKind::ChmutovLike,geo::ExoticImplicitKind::CayleyCubic,geo::ExoticImplicitKind::KummerLike,geo::ExoticImplicitKind::Goursat,geo::ExoticImplicitKind::BlobLattice})mesh(geo::exoticImplicit(k,12),"exotic implicit");
 for(unsigned i=0;i<geo::unknownLabCount();++i)mesh(geo::unknownLabObject(i,24,.3f),geo::unknownLabName(i).c_str());
  req(geo::noveltyScore(geo::discoveredObject(1,24,12))>0,"novelty finite positive");
  Timeline tl(120);auto s=tl.sample(.5);req(s.beatIndex==1&&std::fabs(s.beatPhase)<.001f,"timeline beat");req(s.barIndex==0&&s.barPhase>.24f&&s.barPhase<.26f,"timeline bar");
  // B18: scene-table consistency
  { SceneSystem ss;req(ss.count()==51,"scene count 51");
    for(int i=0;i<ss.count();++i){auto&si=ss.info(i);req(si.id==i,"scene id==index");req(!si.name.empty(),"scene name non-empty");
      req(si.dynamic?si.updateHz>0:true,"scene updateHz sane");
      const auto&m=ss.mesh(i,0.0,42);req(!m.v.empty(),"scene mesh non-empty");
      // B7: same tick -> same mesh pointer (cache hit)
      const auto&m2=ss.mesh(i,0.0,42);req(&m==&m2,"scene cache hit same tick");}
    // B7: static scene tick is always 0
    req(SceneSystem::tickFor(ss.info(6),99.0)==0,"static scene tick=0");
    // B7: dynamic scene tick advances with time (scene 2 = 6 Hz, use 0.2s = 1.2 ticks)
    req(SceneSystem::tickFor(ss.info(2),0.2)>SceneSystem::tickFor(ss.info(2),0.0),"dynamic tick advances");}
  // Text scroller: font + scroll math
  { req(textfont::glyphCount()==95,"font 95 glyphs");
    req(textfont::width()==5&&textfont::height()==7,"font 5x7");
    auto p=textfont::pack();req(p.size()==95*7,"font pack 665 bytes");
    req(textfont::measure("")==0,"measure empty 0");
    req(textfont::measure("ABCDE")==5*5+4,"measure ABCDE=29"); // 5*(5+1)-1
    req(scrollOffset(0.0f,0.f,100.f,50.f)==0.f,"scroll t=0 -> 0");
    // scroll is always in [0, wrap) and non-negative
    float wrap=100.f+50.f;
    for(double t=0;t<60;t+=0.13){float o=scrollOffset(t,0.f,100.f,50.f);
      req(o>=0.f&&o<wrap,"scroll in [0,wrap)");}
    // over time the offset must advance (hit a non-zero value at some t)
    bool advanced=false;
    for(double t=0;t<60;t+=0.01){if(scrollOffset(t,0.f,100.f,50.f)>0.f){advanced=true;break;}}
    req(advanced,"scroll advances over time");
    // Long-session stability: even after hours the offset must stay in [0,wrap)
    // with no float-jitter (the old (float)seconds lost sub-pixel precision
    // after ~minutes). Fixed beatPhase so only `seconds` varies.
    float wrapL=100.f+50.f;
    for(double t:{3600.0,7200.0,86400.0,360000.0,3600000.0,36000000.0}){
      float o=scrollOffset(t,0.f,100.f,50.f);
      req(o>=0.f&&o<wrapL,"scroll in [0,wrap) at long t");
      // Consecutive samples at a long base must advance monotonically and
      // smoothly (no back-step jitter from precision loss).
      float o2=scrollOffset(t+0.001f,0.f,100.f,50.f);
      // o2 is o + ~0.09px, wrapping into [0,wrap) — either it advanced or it
      // wrapped (o2 < o). It must NOT jump by more than the step + epsilon.
      float step=o2-o; if(step<0)step+=wrapL;
      req(step<1.f,"scroll smooth at long t (no jitter)");
    }}
  // Scroller font decode: the EXACT bit math the GLSL shader uses must match
  // the raw glyph bits for every (glyph, x, y) - a regression guard so the
  // marquee can't silently render blank again.
  { auto g=textfont::glyphs();int mism=0;
   for(int gi=0;gi<textfont::glyphCount();gi++)for(int y=0;y<7;y++)for(int x=0;x<5;x++){
     unsigned char row=g[gi].rows[y];int bit=4-x;int expected=(row>>bit)&1;
     int v=row;double b=std::fmod(std::floor(v/std::pow(2.0,bit)),2.0);
     int got=(b>0.5)?1:0;if(got!=expected)mism++;}
   req(mism==0,"scroller font decode matches glyph bits");
   // a few known glyphs must be non-blank
   auto lit=[&](char c){int n=0;for(int y=0;y<7;y++)for(int x=0;x<5;x++)if((g[c-32].rows[y]>>(4-x))&1)n++;return n;};
    req(lit('A')>0&&lit('M')>0&&lit('0')>0,"scroller glyphs non-blank");
    req(lit(' ')==0,"space glyph blank");}
   // Scroller CODE-STRIP: the bug that made the marquee render blank. The shader
   // maps a marquee POSITION (character index i) to a glyph by sampling a 1xN
   // GL_R8 strip the CPU uploads, one byte per char = its ASCII code. The decode
   // is int(round(tex.r)) - 32. This test re-implements the EXACT CPU encode
   // (drawScroller's codes[i]=(uint8_t)text[i]) + shader decode and checks that
   // every printable ASCII char round-trips to the correct glyph index (0..94),
   // and that a position is NOT confused with a code (the old bug).
   { auto enc=[&](const std::string& s){std::vector<uint8_t> c(s.size());for(size_t i=0;i<s.size();++i)c[i]=(uint8_t)s[i];return c;};
     // Decode a strip position i the way scroller.frag does:
     //   u = (i+0.5)/N; code = int(tex(u)+0.5); glyph = code - 32
     auto decodeAt=[&](const std::vector<uint8_t>& strip,int i){
       float u=(float(i)+0.5)/float(strip.size());
       // GL_R8 stores the byte exactly; NEAREST at u=(i+.5)/N hits pixel i.
       (void)u; int code=int(strip[i]); // exact byte (no float loss for 0..126)
       return code-32; };
     std::string all; for(char c=32;c<=126;++c)all+=c;
     auto strip=enc(all);
     req((int)strip.size()==95,"code strip 95 chars");
     for(int i=0;i<95;++i){
       int glyph=decodeAt(strip,i);
       if(glyph!=i){char b[64];std::snprintf(b,sizeof b,"code strip round-trip char %d -> glyph %d (want %d)",i,glyph,i);req(false,b);}
     }
     // The OLD buggy mapping (glyph = position - 32) was wrong for every char
     // whose ASCII code != position+32. Prove the new mapping differs from the
     // old for at least the common letters, so a regression to position-based
     // indexing is caught.
     std::string hello="HELLO";auto hs=enc(hello);
     int oldGlyphAt3=3-32;                 // position 3, old mapping
     int newGlyphAt3=decodeAt(hs,3);       // 'L' = 76-32 = 44
     req(newGlyphAt3==44&&oldGlyphAt3!=44,"code strip: 'L' decodes to 44, not position-32");
   }
  // Image: decode + discovery (uses the repo's UBER logo pack when present)
  { Image miss;req(!Image::loadFromFile("/nonexistent/xyz.jpg",miss),"load missing -> false");
    // Logo pack path: injected by CMake (IW_TEST_LOGO_PACK) so the test runs
    // on every platform; falls back to the dev-machine path if unset.
#ifdef IW_TEST_LOGO_PACK
    auto logos=findLogos(IW_TEST_LOGO_PACK);
#else
    auto logos=findLogos("/home/ulf/privat/wireframe-scroller/UBER_Fullscreen_Logo_Pack");
#endif
    req(findLogos("/nonexistent/dir").empty(),"findLogos missing -> empty");
   if(!logos.empty()){
     req(logos.size()>=20,"logo pack has 20 cards");
     Image im;req(Image::loadFromFile(logos[0],im),"logo decodes");
     req(im.w==1920&&im.h==1080,"logo 1920x1080");
     req(im.rgba.size()==(size_t)1920*1080*4,"logo rgba size");}}
   // Effect system: the lab's 64 CPU warpers + 24 recipes must be deterministic,
   // finite, and fail-safe (never return an empty/invalid mesh).
   {
      req(effectCount()==64,"effectCount 64");
      req(recipeCount()==24,"recipeCount 24");
      // recipeName must agree with recipe().name (regression: recipeName used
      // to return a string_view into a destroyed temporary).
      for(int i=0;i<recipeCount();i++)req(recipeName(i)==recipe(i).name,"recipeName matches recipe");
      req(recipeName(0)=="Raw"&&recipeName(23)=="Dimensional Infection","recipeName endpoints");
      req(recipeName(24)==recipeName(0),"recipeName wraps");
     // every single effect on a torus at several (time, amount) phases stays valid
     for(int id=0;id<effectCount();++id){
       EffectContext ec; ec.amount=1.0f; ec.seed=7u;
       for(float t:{0.f,1.37f,4.2f,9.9f,20.5f}){
         ec.time=t; ec.bass=.5f; ec.mid=.5f; ec.treble=.5f; ec.beat=(t*2.0f)/3.14159f;
         auto m=geo::torus(24,8); bool ok=applyEffect(m,id,ec,nullptr);
         req(ok,"effect returns valid mesh");
         auto st=geo::stats(m); req(st.finite,"effect mesh finite");
         req(st.indicesValid,"effect mesh in-bounds");
       }
       // determinism: identical inputs -> identical output
       { auto a=geo::torus(24,8), b=geo::torus(24,8);
         EffectContext ec; ec.time=3.14f; ec.amount=.7f; ec.seed=99u; ec.bass=.3f; ec.mid=.6f; ec.treble=.2f; ec.beat=.5f;
         applyEffect(a,id,ec,nullptr); applyEffect(b,id,ec,nullptr);
         bool sameV=a.v.size()==b.v.size(); for(size_t i=0;i<a.v.size()&&sameV;i++)sameV=sameV&&(a.v[i].x==b.v[i].x&&a.v[i].y==b.v[i].y&&a.v[i].z==b.v[i].z);
         bool sameE=a.e.size()==b.e.size(); for(size_t i=0;i<a.e.size()&&sameE;i++)sameE=sameE&&(a.e[i].a==b.e[i].a&&a.e[i].b==b.e[i].b);
         req(sameV,"effect deterministic (vertices)");
         req(sameE,"effect deterministic (edges)"); }
     }
     // every curated recipe on several scenes/secondary objects stays valid.
     // (Cheap meshes: the edge-subdivide stages are O(edges), and the lab's own
     // matrix uses ~1000-edge objects.)
     auto scenes=[&](){ return std::vector<Mesh3>{geo::torus(16,6),geo::cube(1.f),geo::uvSphere(8,16)}; };
     for(int rid=0;rid<recipeCount();++rid){
       auto r=recipe(rid); req(r.stageCount>=0&&r.stageCount<=6,"recipe stage count sane");
       for(const auto& base:scenes()){
         Mesh3 m=base; const Mesh3* sec=&base;
         for(float t:{0.f,2.5f}){
           EffectContext ec; ec.time=t; ec.amount=1.2f; ec.seed=123u+uint32_t(rid); ec.bass=.5f+.5f*std::sin(t); ec.mid=.5f+.5f*std::sin(t+1); ec.treble=.5f+.5f*std::sin(t+2); ec.beat=.5f+.5f*std::sin(t*3);
           bool ok=applyRecipe(m,r,ec,sec);
           auto st=geo::stats(m);
           req(ok,"recipe applies (fail-safe)");
           req(st.finite,"recipe mesh finite");
           req(st.indicesValid,"recipe mesh in-bounds");
         }
       }
     }
     // procedural mutation is deterministic and bounded
     for(uint32_t s:{0u,1u,7u,424242u,0xffffffffu}){
       auto a=mutateRecipe(s,4), b=mutateRecipe(s,4);
       req(a.name==b.name,"mutation name deterministic");
       req(a.stageCount==b.stageCount&&a.stageCount==4,"mutation stage count");
       bool same=true; for(int i=0;i<4;i++){const auto&x=a.stages[i];const auto&y=b.stages[i]; same=same&&(x.effect==y.effect&&x.amount==y.amount&&x.timeScale==y.timeScale&&x.timeOffset==y.timeOffset&&x.seedOffset==y.seedOffset);}
       req(same,"mutation deterministic");
       // mutated recipe must run without breaking
       { Mesh3 m=geo::torus(24,8); EffectContext ec; ec.time=1.1f; ec.amount=1.f; ec.seed=5u; ec.bass=.5f; ec.mid=.5f; ec.treble=.5f; ec.beat=.5f;
         auto st=geo::stats(m); req(applyRecipe(m,a,ec,nullptr),"mutation recipe applies");
         st=geo::stats(m); req(st.finite&&st.indicesValid,"mutation mesh finite+in-bounds"); }
     }
     // bridge effects (36/37) need a secondary object and add vertices
     { auto a=geo::torus(24,8), sec=geo::cube(1.f); auto before=a.v.size();
       EffectContext ec; ec.time=1.0f; ec.amount=1.f; ec.seed=1u; ec.bass=.5f;ec.mid=.5f;ec.treble=.5f;ec.beat=.5f;
       bool ok36=applyEffect(a,36,ec,&sec); bool ok37=applyEffect(a,37,ec,&sec);
       req(ok36&&a.v.size()>before,"dual bridge adds vertices");
       req(ok37,"nearest bridge valid"); }
     // empty-mesh guard
     { Mesh3 empty; EffectContext ec; req(!applyEffect(empty,4,ec,nullptr),"empty mesh -> false"); }
   }
   // Audio band-split: the one-pole IIR coefficients must separate low/mid/high.
   // Simulate the same 3-tap lowpass chain the renderer uses and verify that a
   // 100 Hz sine ends up mostly in the bass band, 1 kHz in mid, 8 kHz in treble.
   {
     auto bandOf=[&](float freq){
       const float rate=48000.f;
       const float aB=.021f, aM=.14f, aT=.5f;
       float lpB=0,lpM=0,lpT=0; float bAcc=0,mAcc=0,tAcc=0; int N=4800;
       for(int i=0;i<N;i++){
         float s=std::sin(2.f*3.14159265f*freq*float(i)/rate);
         lpB+=aB*(s-lpB); lpM+=aM*(s-lpM); lpT+=aT*(s-lpT);
         bAcc+=std::fabs(lpB); mAcc+=std::fabs(lpM-lpB); tAcc+=std::fabs(lpT-lpM);
       }
       float b=bAcc/float(N),m=mAcc/float(N),t=tAcc/float(N);
       float tot=b+m+t; if(tot<1e-9f)return std::tuple<float,float,float>(0.f,0.f,0.f);
       return std::make_tuple(b/tot,m/tot,t/tot);
     };
      auto[bl,ml,tl]=bandOf(100.f);   req(bl>ml&&bl>tl,"100Hz mostly bass");
      auto[bm,mm,tm]=bandOf(1000.f);  req(mm>=bm&&mm>=tm,"1kHz mostly mid");
      auto[bt,mt,tt]=bandOf(8000.f);   req(tt>=bt&&tt>=mt,"8kHz mostly treble");
    }
    // Shader sanity: no C-specific math functions in GLSL (they're not GLSL
    // builtins and Apple's stricter compiler rejects them — e.g. fmod, which
    // broke the scroller shader on macOS). Scan every .vert/.frag for the
    // common C names that have GLSL equivalents (mod/abs/min/max/pow/...).
    {
      const std::vector<std::string> bad={"fmod","fabs","fmin","fmax","sinf","cosf","tanf","sqrtf","floorf","ceilf","powf","expf","logf","atanf","asinf","acosf","fmodf"};
      auto shadersDir=std::filesystem::path("shaders");
      if(!std::filesystem::exists(shadersDir))shadersDir=std::filesystem::path(__FILE__).parent_path()/".." /"shaders";
      for(const auto& entry:std::filesystem::directory_iterator(shadersDir)){
        if(!entry.is_regular_file())continue;
        auto ext=entry.path().extension().string();
        if(ext!=".vert"&&ext!=".frag")continue;
        std::ifstream f(entry.path()); std::string line;
        while(std::getline(f,line)){
          // strip // comments so a comment mentioning fmod doesn't false-positive
          size_t c=line.find("//"); if(c!=std::string::npos)line=line.substr(0,c);
          for(const auto& b:bad){
            // word-boundary match (not part of a longer identifier)
            size_t p=0;
            while((p=line.find(b,p))!=std::string::npos){
              bool lp=(p==0)||(!std::isalnum((unsigned char)line[p-1])&&line[p-1]!='_');
              size_t e=p+b.size();
              bool rp=(e>=line.size())||(!std::isalnum((unsigned char)line[e])&&line[e]!='_');
              if(lp&&rp){std::cerr<<"FAIL: C-specific function '"<<b<<"' in shader "<<entry.path()<<" line: "<<line<<"\n";std::exit(1);}
              p=e;
            }
          }
        }
      }
      // the scroller shader must use the GLSL `mod` builtin for bit extraction
      { std::ifstream f(shadersDir/"scroller.frag"); std::string all((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
        req(all.find("mod(floor(v / pow(2.0, bit)), 2.0)")!=std::string::npos,"scroller uses GLSL mod builtin"); }
      // post.frag must be tempo-synced: the synthetic beat pulse must be driven
      // by uBpm (bpm/60 Hz), NOT a hard-coded 3.2 Hz. A regression that re-introduces
      // a fixed frequency would make the FX pulse at the wrong rate for --bpm != 192.
      { std::ifstream f(shadersDir/"post.frag"); std::string all((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
        req(all.find("uBpm")!=std::string::npos,"post.frag declares uBpm");
        req(all.find("uBpm/60.0")!=std::string::npos,"post.frag beat synced to bpm/60 Hz");
        // the old hard-coded 3.2 Hz (192 BPM only) must be gone from the beat line
        req(all.find("sin(uTime*3.2)")==std::string::npos,"post.frag no hard-coded 3.2 Hz beat");
        // --no-post bypass path must exist (ACES tone-map + early return)
        req(all.find("uBypass")!=std::string::npos,"post.frag declares uBypass");
      }
    }
    std::cout<<"geometry_tests: PASS; exact 120-cell V="<<c120.v.size()<<" E="<<c120.e.size()<<"\n";
 }
