#include "Geometry.hpp"
#include "AdvancedGeometry.hpp"
#include "Image.hpp"
#include "Scene.hpp"
#include "TextScroller.hpp"
#include "Timeline.hpp"
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
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
    req(advanced,"scroll advances over time");}
  // Image: decode + discovery (uses the repo's UBER logo pack when present)
  { Image miss;req(!Image::loadFromFile("/nonexistent/xyz.jpg",miss),"load missing -> false");
   auto logos=findLogos("/home/ulf/privat/wireframe-scroller/UBER_Fullscreen_Logo_Pack");
   req(findLogos("/nonexistent/dir").empty(),"findLogos missing -> empty");
   if(!logos.empty()){
     req(logos.size()>=20,"logo pack has 20 cards");
     Image im;req(Image::loadFromFile(logos[0],im),"logo decodes");
     req(im.w==1920&&im.h==1080,"logo 1920x1080");
     req(im.rgba.size()==(size_t)1920*1080*4,"logo rgba size");}}
  std::cout<<"geometry_tests: PASS; exact 120-cell V="<<c120.v.size()<<" E="<<c120.e.size()<<"\n";
}
