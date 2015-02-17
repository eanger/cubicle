#include <cassert>
#include <cmath>
#include <iostream>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>
#include "SDL.h"
#include "SDL_image.h"
#include "assets.h"
using namespace std;
using namespace glm;

#define GUY_WIDTH 32
#define GUY_HEIGHT 35
#define GUY_START_PIXEL 448
#define CHAIR_WIDTH 124
#define CHAIR_HEIGHT 112
#define CHAIR_START_PIXEL 574

namespace {
enum class Action {
  LEFT,
  RIGHT,
  UP,
  DOWN,
  SHOOT
};

struct Entity {
  vec2 pos;
  vec2 vel;
};

struct Renderable {
  struct AnimationFrame{
    ivec2 offset;
    AnimationFrame(ivec2 offset) : offset{offset} {}
  };
  int display_width, display_height;
  ivec2 spritesheet_offset;
  int sprite_width, sprite_height;
  vector<AnimationFrame> frames;
  int cur_frame_idx;
  int cur_frame_tick_count;
};

struct State {
  bool is_done;
  set<Action> actions;
  int next_entity_idx;
  unordered_map<int, Entity> entities;
  int screen_w, screen_h;
  unordered_map<int, Renderable> renderables;
  unordered_set<int> mobs;
  SDL_Renderer* renderer;
  SDL_Texture* spritesheet;

  State() : is_done{false}, next_entity_idx{0}, screen_w{800}, screen_h{600} {
    auto& hero = entities[next_entity_idx++];
    hero.pos = vec2(0.0f);
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
    auto surf = IMG_Load(get_asset_path("spritesheet.bmp").c_str());
    if(!surf){
      cerr << "Bad img load: " << IMG_GetError() << endl;
      exit(-1);
    }
    spritesheet = SDL_CreateTextureFromSurface(renderer, surf);
    if(!spritesheet){
      cerr << "Bad convert to texture: " << SDL_GetError() << endl;
      exit(-1);
    }
    auto& hero_rend = renderables[0];
    hero_rend.display_width = hero_rend.sprite_width = GUY_WIDTH;
    hero_rend.display_height = hero_rend.sprite_height = GUY_HEIGHT;
    hero_rend.spritesheet_offset = {GUY_START_PIXEL, 0};
    hero_rend.frames.push_back(Renderable::AnimationFrame({0,0}));
    hero_rend.frames.push_back(Renderable::AnimationFrame({46,0}));
    hero_rend.frames.push_back(Renderable::AnimationFrame({92,0}));
    hero_rend.frames.push_back(Renderable::AnimationFrame({46,0}));
    hero_rend.cur_frame_idx = 0;
    hero_rend.cur_frame_tick_count = 0;
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
            world_data.actions.insert(Action::LEFT);
          } break;
          case SDLK_RIGHT: {
            world_data.actions.insert(Action::RIGHT);
          } break;
          case SDLK_UP: {
            world_data.actions.insert(Action::UP);
          } break;
          case SDLK_DOWN: {
            world_data.actions.insert(Action::DOWN);
          } break;
          case SDLK_SPACE: {
            world_data.actions.insert(Action::SHOOT);
          } break;
        }
      } break;
      case SDL_KEYUP: {
        switch(event.key.keysym.sym){
          case SDLK_LEFT: {
            world_data.actions.erase(Action::LEFT);
          } break;
          case SDLK_RIGHT: {
            world_data.actions.erase(Action::RIGHT);
          } break;
          case SDLK_UP: {
            world_data.actions.erase(Action::UP);
          } break;
          case SDLK_DOWN: {
            world_data.actions.erase(Action::DOWN);
          } break;
        }
      } break;
    }
  }
}

void make_mob_rend(Renderable& rend){
  rend.display_width = GUY_WIDTH;
  rend.display_height = GUY_HEIGHT;
  rend.spritesheet_offset = {CHAIR_START_PIXEL, 0};
  rend.sprite_width = CHAIR_WIDTH;
  rend.sprite_height = CHAIR_HEIGHT;
  rend.frames.push_back(Renderable::AnimationFrame({0,0}));
  rend.cur_frame_idx = 0;
  rend.cur_frame_tick_count = 0;
}

#define PLAYER_SPEED 5.0f
#define MOB_SPEED 4.0f
#define MOB_MASS 50
#define MAX_FORCE 5.0f
bool update_logic(State& world_data) {
  vec2 vel = {0,0};
  auto& hero = world_data.entities[0];
  for(const auto& action : world_data.actions){
    switch(action){
      case Action::LEFT: {
        vel.x = -1;
      } break;
      case Action::RIGHT: {
        vel.x = 1;
      } break;
      case Action::UP: {
        vel.y = -1;
      } break;
      case Action::DOWN: {
        vel.y = 1;
      } break;
      case Action::SHOOT: {
        // generate new chair mob
        auto new_mob_idx = world_data.next_entity_idx++;
        auto& new_mob = world_data.entities[new_mob_idx];
        new_mob.pos = {0,0};
        new_mob.vel = {0,0};
        auto& new_mob_rend = world_data.renderables[new_mob_idx];
        make_mob_rend(new_mob_rend);
        world_data.mobs.insert(new_mob_idx);
        world_data.actions.erase(Action::SHOOT);
      } break;
    }
  }
  if(length(vel) > 0){
    hero.pos += normalize(vel) * PLAYER_SPEED;
  }

  // move mobs towards player
  for(const auto& mob_idx : world_data.mobs){
    auto& mob = world_data.entities[mob_idx];
    auto desired_vel = hero.pos - mob.pos;
    if(length(desired_vel) > 0){
      desired_vel = normalize(desired_vel) * MOB_SPEED;
    }
    auto steering = desired_vel - mob.vel;
    if(length(steering) > MAX_FORCE){
      steering = normalize(steering) * MAX_FORCE;
    }
    steering /= MOB_MASS;
    mob.vel = mob.vel + steering;
    if(length(mob.vel) > MOB_SPEED){
      mob.vel = normalize(mob.vel) * MOB_SPEED;
    }
    mob.pos += mob.vel;
  }

  return world_data.is_done;
}

#define TICKS_PER_FRAME 20
void render(State& world_data) {
  SDL_SetRenderDrawColor(world_data.renderer, 255, 255, 255, 0);
  SDL_RenderClear(world_data.renderer);
  for(auto& rend_item : world_data.renderables){
    auto& entity = world_data.entities[rend_item.first];
    auto& rend = rend_item.second;
    ivec2 offset = rend.spritesheet_offset + rend.frames[rend.cur_frame_idx].offset;
    SDL_Rect srcrect{offset.x, offset.y, rend.sprite_width, rend.sprite_height};
    SDL_Rect destrect{(int)entity.pos.x, (int)entity.pos.y, rend.display_width, rend.display_height};
    SDL_RenderCopy(world_data.renderer, world_data.spritesheet, &srcrect, &destrect);

    ++rend.cur_frame_tick_count;
    if(rend.cur_frame_tick_count == TICKS_PER_FRAME){
      if(rend.cur_frame_idx == rend.frames.size() - 1){
        rend.cur_frame_idx = 0;
      } else {
        ++rend.cur_frame_idx;
      }
      rend.cur_frame_tick_count = 0;
    }
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

