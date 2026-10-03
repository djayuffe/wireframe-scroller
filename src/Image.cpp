#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION
#include "Image.hpp"
#include <algorithm>
#include <filesystem>
namespace fs=std::filesystem;
bool Image::loadFromFile(const std::string& path,Image& out){
  fs::path p(path);
  std::error_code ec;
  if(!fs::exists(p,ec)||ec) return false;
  int ch=0;
  uint8_t* px=stbi_load(p.string().c_str(),&out.w,&out.h,&ch,4);
  if(!px) return false;
  size_t n=(size_t)out.w*out.h*4;
  out.rgba.assign(px,px+n);
  stbi_image_free(px);
  return !out.empty();
}
std::vector<std::string> findLogos(const std::string& dir){
  std::vector<std::string> names;
  fs::path d(dir);
  std::error_code ec;
  if(!fs::is_directory(d,ec)||ec) return names;
  for(auto& e:fs::directory_iterator(d,ec)){
    std::error_code ec2;
    if(!e.is_regular_file(ec2)||ec2) continue;
    std::string fn=e.path().filename().string();
    // Match UBER_<n>_<...>_1920x1080.jpg (the 20 scene cards).
    if(fn.rfind("UBER_",0)==0&&fn.find("_1920x1080.jpg")!=std::string::npos){
      names.push_back(e.path().string());
    }
  }
  std::sort(names.begin(),names.end());
  return names;
}
