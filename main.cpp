#include <cassert>
#include <iostream>
#include <queue>
#include <unordered_map>
#include "SDL.h"
#include "SDL_image.h"
#include "assets.h"
using namespace std;

namespace {
enum class Action {
  LEFT,
  RIGHT,
  UP,
  DOWN,
  SHOOT
};

struct Entity {
  unsigned x, y;
};

struct Renderable {
  SDL_Texture* texture;
  unsigned width, height;
};

struct State {
  bool is_done;
  queue<Action> actions;
  unsigned next_entity_idx;
  unordered_map<unsigned, Entity> entities;
  unsigned screen_w, screen_h;
  unordered_map<unsigned, Renderable> renderables;
  SDL_Renderer* renderer;

  State() : is_done{false}, next_entity_idx{0}, screen_w{800}, screen_h{600} {
    auto hero = entities[next_entity_idx++];
    hero.x = hero.y = 0;
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    auto window = SDL_CreateWindow("game",
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   screen_w,
                                   screen_h,
                                   SDL_WINDOW_OPENGL);
    renderer = SDL_CreateRenderer(window,
                                  -1 /* first available */,
                                  SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    auto hero_renderable = renderables[0];
    auto surf = IMG_Load(get_asset_path("guy.bmp").c_str());
    if(!surf){
      cerr << "Bad img load: " << IMG_GetError() << endl;
      exit(-1);
    }
    hero_renderable.texture = SDL_CreateTextureFromSurface(renderer, surf);
    hero_renderable.width = surf->w;
    hero_renderable.height = surf->h;
  }
};

void read_input(State& world_data) {
  SDL_Event event;
  while(SDL_PollEvent(&event)){
    switch(event.type){
      case SDL_QUIT: {
        world_data.is_done = true;
      } break;
      case SDL_KEYDOWN: {
        switch(event.key.keysym.sym){
          case SDLK_q: {
            world_data.is_done = true;
          } break;
          case SDLK_LEFT: {
            world_data.actions.push(Action::LEFT);
          } break;
          case SDLK_RIGHT: {
            world_data.actions.push(Action::RIGHT);
          } break;
          case SDLK_UP: {
            world_data.actions.push(Action::UP);
          } break;
          case SDLK_DOWN: {
            world_data.actions.push(Action::DOWN);
          } break;
          case SDLK_SPACE: {
            world_data.actions.push(Action::SHOOT);
          } break;
          default: {
            cerr << "Invalid keypress\n";
          } break;
        }
      } break;
      default: {
        cerr << "Invalid event type\n";
      }
    }
  }
}

bool update_logic(State& world_data) {
  while(!world_data.actions.empty()){
    auto action = world_data.actions.front();
    world_data.actions.pop();
    auto& hero = world_data.entities[0];
    switch(action){
      case Action::LEFT: {
        hero.x = hero.x == 0 ? hero.x : hero.x - 1;
      } break;
      case Action::RIGHT: {
        hero.x = hero.x == world_data.screen_w - 1 ? hero.x : hero.x + 1;
      } break;
      case Action::UP: {
        hero.y = hero.y == 0 ? hero.x : hero.y - 1;
      } break;
      case Action::DOWN: {
        hero.y = hero.y == world_data.screen_h - 1 ? hero.y : hero.y + 1;
      } break;
      case Action::SHOOT: {
      } break;
    }
  }
  return world_data.is_done;
}

void render(State& world_data) {
  SDL_SetRenderDrawColor(world_data.renderer, 255, 255, 255, 0);
  SDL_RenderClear(world_data.renderer);
  for(auto& rend : world_data.renderables){
    auto& entity = world_data.entities[rend.first];
    SDL_Rect rect{static_cast<int>(entity.x), static_cast<int>(entity.y), static_cast<int>(rend.second.width), static_cast<int>(rend.second.height)};
    SDL_RenderCopy(world_data.renderer, rend.second.texture, nullptr, nullptr);
  }
  SDL_RenderPresent(world_data.renderer);
}
}

int main(int argc, char** argv) {
  bool done = false;
  State world_data;
  while(!done){
    read_input(world_data);
    done = update_logic(world_data);
    render(world_data);
  }
  cout << "Game over\n";
  return 0;
}

