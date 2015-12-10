#pragma once

#include <string>

#include "SDL.h"

inline std::string get_bin_path() {
  static std::string bin_path = SDL_GetBasePath();
  return bin_path;
}

inline std::string get_asset_path(std::string name) {
  auto bin_path = get_bin_path();
  return bin_path + "../assets/" + name;
}

inline std::string get_lib_path(std::string lib_name) {
  auto bin_path = get_bin_path();
  return bin_path + "/" + lib_name;
}
