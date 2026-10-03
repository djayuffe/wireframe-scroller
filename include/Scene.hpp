#pragma once
#include "Geometry.hpp"
#include <cstdint>
#include <string_view>
struct SceneInfo { int id; std::string_view name; bool dynamic; double updateHz; std::string_view provenance; };
class SceneSystem {
public:
 SceneSystem();
 const SceneInfo& info(int id) const;
 int count() const;
 const Mesh3& mesh(int id,double seconds,uint64_t seed);
private:
 Polytope4 c600_,c120_; Mesh3 cache_; int cachedId_=-1; uint64_t cachedTick_=~uint64_t(0);
};
