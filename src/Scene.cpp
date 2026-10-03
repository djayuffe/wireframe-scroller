#include "Scene.hpp"
#include "AdvancedGeometry.hpp"
#include <algorithm>
#include <array>
#include <cmath>
namespace {
constexpr std::array<SceneInfo,51> S{{
 {0,"600-cell projection",true,60,"exact 600-cell + 4D projection"},{1,"600-cell slice",true,30,"exact face/hyperplane intersection"},
 {2,"Gyroid",true,6,"numerical implicit"},{3,"Schwarz P",true,6,"numerical implicit"},{4,"Schwarz D",true,6,"numerical implicit"},{5,"Neovius",true,6,"numerical implicit"},
 {6,"Hopf fibres",false,1,"parametric S3 stereographic projection"},{7,"Boy surface",false,1,"analytic immersion"},{8,"Superformula",true,15,"parametric"},
 {9,"120-cell projection",true,60,"exact dual incidence + 4D projection"},{10,"Clifford torus",false,1,"parametric S3 stereographic projection"},
 {11,"Hyperbolic ball",false,1,"artistic Poincare-ball visualization"},{12,"Quaternion Julia slice",true,3,"numerical escape-boundary lattice"},
 {13,"Lissajous knot",true,15,"parametric curve"},{14,"Lorenz attractor",false,1,"numerical trajectory"},{15,"Discovered object",true,.125,"deterministic seeded harmonic surface"},
 {16,"Mobius strip",false,1,"parametric non-orientable surface"},{17,"Klein bottle",false,1,"parametric immersion"},{18,"Enneper surface",false,1,"minimal surface"},{19,"Helicoid",false,1,"minimal ruled surface"},{20,"Catenoid",false,1,"minimal surface"},{21,"Dini surface",false,1,"constant-negative-curvature surface"},{22,"Pseudosphere",false,1,"tractricoid surface"},{23,"Roman surface",false,1,"Steiner RP2 immersion"},{24,"Cross-cap",false,1,"RP2 immersion"},{25,"Torus knot",true,15,"parametric algebraic knot"},{26,"Viviani curve",false,1,"sphere-cylinder intersection curve"},{27,"Spherical spiral",false,1,"parametric spherical loxodrome-like wire"},{28,"Hypotrochoid knot",true,15,"parametric spatial roulette"},{29,"Duffing attractor",false,1,"numerical chaotic trajectory"},{30,"Rossler attractor",false,1,"numerical chaotic trajectory"},{31,"Thomas attractor",false,1,"numerical cyclic chaotic trajectory"},{32,"Sierpinski tetrahedron",false,1,"recursive fractal tetrahedral complex"},{33,"Heart algebraic",true,4,"numerical algebraic isosurface"},{34,"Barth sextic-like",false,1,"icosahedral-inspired sextic visualization"},{35,"Tanglecube",true,4,"numerical quartic isosurface"},{36,"Chmutov-like",false,1,"Chebyshev-inspired algebraic visualization"},{37,"Cayley cubic",false,1,"numerical cubic isosurface"},{38,"Kummer-like",false,1,"quartic nodal-inspired visualization"},{39,"Goursat surface",false,1,"numerical quartic isosurface"},{40,"Blob lattice",true,4,"periodic implicit field"},
 {41,"Unknown supercage crown",true,12,"ported unknown-wireframe-lab procedural object"},{42,"Unknown knot-bundle reactor",true,12,"ported unknown-wireframe-lab procedural object"},
 {43,"Unknown phyllotaxis vortex",true,12,"ported unknown-wireframe-lab procedural object"},{44,"Unknown ruled singularity fan",true,12,"ported unknown-wireframe-lab procedural object"},
 {45,"Unknown prime-lobed lantern",true,12,"ported unknown-wireframe-lab procedural object"},{46,"Unknown aperiodic orbit nest",true,12,"ported unknown-wireframe-lab procedural object"},
 {47,"Unknown superformula organism",true,12,"ported unknown-wireframe-lab procedural object"},{48,"Unknown knot lattice 13",true,12,"ported unknown-wireframe-lab procedural object"},
 {49,"Unknown golden spiral skeleton",true,12,"ported unknown-wireframe-lab procedural object"},{50,"Unknown twisted ruled shell",true,12,"ported unknown-wireframe-lab procedural object"}
}};
}
SceneSystem::SceneSystem():c600_(geo::cell600()),c120_(geo::cell120Vertices()){}
const SceneInfo& SceneSystem::info(int id)const{return S[std::clamp(id,0,int(S.size()-1))];}
int SceneSystem::count()const{return int(S.size());}
const Mesh3& SceneSystem::mesh(int id,double seconds,uint64_t seed){
 id=std::clamp(id,0,count()-1);auto& si=info(id);uint64_t tick=si.dynamic?uint64_t(std::floor(std::max(0.0,seconds)*si.updateHz)):0;
 if(id==cachedId_&&tick==cachedTick_) return cache_;
 float t=float(seconds);
 switch(id){case 0:cache_=geo::project4D(c600_,t*.17f,t*.29f,3.4f,t*.09f,t*.13f);break;case 1:cache_=geo::sliceFaces4D(c600_,{.31f,.47f,.59f,.57f},std::sin(t*.31f)*1.45f);break;
 case 2:cache_=geo::implicitSurface(geo::ImplicitKind::Gyroid,25,std::sin(t*.22f)*.25f,t*.08f);break;case 3:cache_=geo::implicitSurface(geo::ImplicitKind::SchwarzP,25,0,t*.1f);break;case 4:cache_=geo::implicitSurface(geo::ImplicitKind::SchwarzD,24,0,t*.08f);break;case 5:cache_=geo::implicitSurface(geo::ImplicitKind::Neovius,24,0,t*.07f);break;
 case 6:cache_=geo::hopfFibres(42,112,2.3f);break;case 7:cache_=geo::boySurface(84,44,.95f);break;case 8:cache_=geo::superformula(96,48,5.f+2.f*std::sin(t*.13f),3.f+2.f*std::sin(t*.17f),.38f,1.6f,1.6f);break;
 case 9:cache_=geo::project4D(c120_,t*.09f,t*.13f,8.5f,t*.07f,t*.11f);break;case 10:cache_=geo::cliffordTorus(56,30,2.2f);break;case 11:cache_=geo::hyperbolicBall(7,20,.9f);break;
 case 12:cache_=geo::quaternionJulia(24,8,.055f,{-.2f,.7f,.05f*std::sin(t*.1f),0});break;case 13:cache_=geo::lissajousKnot(1800,3,4,7,.4f+t*.03f);break;case 14:cache_=geo::strangeAttractor(14000,.004f);break;case 15:cache_=geo::discoveredObject(seed+tick,88,44);break;
 case 16:cache_=geo::mobiusStrip();break;case 17:cache_=geo::kleinBottle();break;case 18:cache_=geo::enneperSurface();break;case 19:cache_=geo::helicoidSurface();break;case 20:cache_=geo::catenoidSurface();break;case 21:cache_=geo::diniSurface();break;case 22:cache_=geo::pseudosphere();break;case 23:cache_=geo::romanSurface();break;case 24:cache_=geo::crossCap();break;case 25:cache_=geo::torusKnot(1800,3,7,.72f,.28f);break;case 26:cache_=geo::vivianiCurve();break;case 27:cache_=geo::sphericalSpiral();break;case 28:cache_=geo::hypotrochoidKnot(1800,5,3,5+.3f*std::sin(t*.2f));break;case 29:cache_=geo::duffingAttractor();break;case 30:cache_=geo::rosslerAttractor();break;case 31:cache_=geo::thomasAttractor();break;case 32:cache_=geo::sierpinskiTetrahedron(5);break;case 33:cache_=geo::exoticImplicit(geo::ExoticImplicitKind::Heart,25,t*.1f);break;case 34:cache_=geo::exoticImplicit(geo::ExoticImplicitKind::BarthSexticLike,25);break;case 35:cache_=geo::exoticImplicit(geo::ExoticImplicitKind::TangleCube,25,t*.1f);break;case 36:cache_=geo::exoticImplicit(geo::ExoticImplicitKind::ChmutovLike,25);break;case 37:cache_=geo::exoticImplicit(geo::ExoticImplicitKind::CayleyCubic,25);break;case 38:cache_=geo::exoticImplicit(geo::ExoticImplicitKind::KummerLike,25);break;case 39:cache_=geo::exoticImplicit(geo::ExoticImplicitKind::Goursat,25);break;case 40:cache_=geo::exoticImplicit(geo::ExoticImplicitKind::BlobLattice,25,t*.12f);break;
 case 41:case 42:case 43:case 44:case 45:case 46:case 47:case 48:case 49:case 50:cache_=geo::unknownLabObject(unsigned(id-41),56,t);break;default:cache_=geo::discoveredObject(seed+tick,88,44);break;}
 cachedId_=id;cachedTick_=tick;return cache_;
}
