#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <iterator>
#include <queue>
#include <random>
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
using namespace std::chrono;
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
  SHOOT,
  CLICK,
  RESET
};

struct Entity {
  vec2 pos;
  vec2 vel;
};

struct Renderable {
  int display_width, display_height;
  ivec2 spritesheet_offset;
  int sprite_width, sprite_height;
  vector<int> frames;
  vector<int> directions;
  int facing_idx;
  int cur_frame_idx;
  float cur_frame_tick_count;
};

struct Task {
  vec2 position;
  enum class Chore {
    IDLE,
    BUILD_CHAIR
  } chore;

  Task() : chore(Chore::IDLE) {}
  Task(vec2 p, Chore c) : position{p}, chore{c} {}
};

struct State {
  bool is_done;
  set<Action> actions;
  int next_entity_idx;
  unordered_map<int, Entity> entities;
  int screen_w, screen_h;
  unordered_map<int, Renderable> renderables;
  unordered_map<int, Task> workers;
  SDL_Renderer* renderer;
  SDL_Texture* spritesheet;
  random_device rd;
  mt19937 mt;
  uniform_real_distribution<float> rand_x;
  uniform_real_distribution<float> rand_y;
  vec2 mouse_pos;
  queue<Task> tasks;

  void clear() {
    next_entity_idx = 0;
    entities.clear();
    renderables.clear();
    workers.clear();
    tasks = queue<Task>();
  }

  State()
      : is_done{false},
        next_entity_idx{0},
        screen_w{800},
        screen_h{600},
        mt{rd()},
        rand_x(0, screen_w),
        rand_y(0, screen_h) {
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
          case SDLK_r: {
            world_data.actions.insert(Action::RESET);
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
      // event.motion.x,y
      case SDL_MOUSEBUTTONDOWN: {
        world_data.actions.insert(Action::CLICK);
        world_data.mouse_pos = {event.motion.x, event.motion.y};
      } break;
    }
  }
}

int make_chair(State& world_data, ivec2 pos) {
  auto new_chair_idx = world_data.next_entity_idx++;
  auto& new_chair = world_data.entities[new_chair_idx];
  new_chair.pos = pos;
  new_chair.vel = {0,0};
  auto& rend = world_data.renderables[new_chair_idx];
  rend.display_width = GUY_WIDTH;
  rend.sprite_width = CHAIR_WIDTH;
  rend.display_height = GUY_HEIGHT;
  rend.sprite_height = CHAIR_HEIGHT;
  rend.spritesheet_offset = {CHAIR_START_PIXEL, 0};
  rend.directions.emplace_back(0);
  rend.frames.emplace_back(0);
  rend.facing_idx = 0;
  rend.cur_frame_idx = 0;
  rend.cur_frame_tick_count = 0;
}

void make_worker_rend(Renderable& rend) {
  rend.display_width = rend.sprite_width = GUY_WIDTH;
  rend.display_height = rend.sprite_height = GUY_HEIGHT;
  rend.spritesheet_offset = {GUY_START_PIXEL, 0};
  rend.directions.emplace_back(0);
  rend.directions.emplace_back(37);
  rend.directions.emplace_back(73);
  rend.directions.emplace_back(108);
  rend.frames.emplace_back(0);
  rend.frames.emplace_back(46);
  rend.frames.emplace_back(92);
  rend.frames.emplace_back(46);
  rend.facing_idx = 0;
  rend.cur_frame_idx = 0;
  rend.cur_frame_tick_count = 0;
}

template<typename T>
bool isNonZero(const T& val) {
  return val != T(0);
}

template<typename T>
T truncate(const T& val, float max_len) {
  auto res = val;
  if(isNonZero(val) && length(val) > max_len){
    res = normalize(res) * max_len;
  }
  return res;
}

#define MOB_SPEED 300.0f
#define MOB_MASS 2.0f
#define MAX_FORCE 8.0f
#define MAX_SEE_AHEAD 50
#define MOB_COLLIDE_SIZE 50
#define FRICTION_COEFF 10.0f
#define DISTANCE_PER_FRAME 20
#define SLOWING_RADIUS 10.0f
#define ARRIVAL_RADIUS 5.0f
bool update_logic(float dt, State& world_data) {
  for(const auto& action : world_data.actions){
    switch(action){
      case Action::RESET: {
        world_data.clear();
      } break;
      case Action::SHOOT: {
        // generate new worker
        auto new_worker_idx = world_data.next_entity_idx++;
        auto& new_worker = world_data.entities[new_worker_idx];
        new_worker.pos = {world_data.rand_x(world_data.mt), world_data.rand_y(world_data.mt)};
        new_worker.vel = {0,0};
        auto& new_worker_rend = world_data.renderables[new_worker_idx];
        make_worker_rend(new_worker_rend);
        world_data.workers[new_worker_idx];
      } break;
      case Action::CLICK: {
        // generate new chair task
        world_data.tasks.push({world_data.mouse_pos, Task::Chore::BUILD_CHAIR});
      } break;
    }
  }
  world_data.actions.clear();

  // move workers towards target
  for(auto& worker_iter : world_data.workers){
    const auto& id = worker_iter.first;
    auto& task = worker_iter.second;
    switch(task.chore){
      case Task::Chore::IDLE: {
        if(!world_data.tasks.empty()){
          task = world_data.tasks.front();
          world_data.tasks.pop();
        }
      } break;
      case Task::Chore::BUILD_CHAIR: {
        auto& worker = world_data.entities[id];
        auto target = task.position;
        auto desired_vel = target - worker.pos;

        // collision avoidance
        if(isNonZero(worker.vel)){
          auto ahead = worker.pos + truncate(worker.vel, MAX_SEE_AHEAD);
          float closest = MAX_SEE_AHEAD;
          vec2 avoid_force;
          for(const auto& collide_iter : world_data.workers){
            const auto& collide_idx = collide_iter.first;
            const auto& collide = world_data.entities[collide_idx];
            if(&collide == &worker){ continue; }
            if(length(ahead - collide.pos) > MOB_COLLIDE_SIZE){ continue; }
            if(distance(worker.pos, collide.pos) < closest){
              closest = distance(worker.pos, collide.pos);
              avoid_force = ahead - collide.pos;
            }
          }
          desired_vel += avoid_force;
        }

        // arrival
        auto dist = distance(worker.pos, target);
        assert(!isnan(dist) && "invalid distance between worker and target");
        if(dist < SLOWING_RADIUS){
          auto slowing_amount = clamp(dist / SLOWING_RADIUS, 0.1f, 1.0f);
          desired_vel *= slowing_amount;
        }

        auto steering_force = truncate(desired_vel - worker.vel, MAX_FORCE);
        auto steering_vel = steering_force / MOB_MASS;
        worker.vel = truncate(worker.vel + steering_vel, MOB_SPEED);
        // down right up left
        float dot_prods[4] = {dot(worker.vel, {0, 1}),  dot(worker.vel, {1, 0}),
                              dot(worker.vel, {0, -1}), dot(worker.vel, {-1, 0})};
        auto max_elem = max_element(begin(dot_prods), end(dot_prods));
        auto facing_idx = std::distance(begin(dot_prods), max_elem);
        if(isNonZero(worker.vel)){
          auto& rend = world_data.renderables[id];
          rend.facing_idx = facing_idx;
          rend.cur_frame_tick_count += length(worker.vel * dt);
          if(rend.cur_frame_tick_count >= DISTANCE_PER_FRAME){
            if(rend.cur_frame_idx == rend.frames.size() - 1){
              rend.cur_frame_idx = 0;
            } else {
              ++rend.cur_frame_idx;
            }
            rend.cur_frame_tick_count = 0;
          }
        }  
        worker.pos += worker.vel * dt;
        if(dist < ARRIVAL_RADIUS){
          worker.vel = {0,0};
          make_chair(world_data, target);
          task.chore = Task::Chore::IDLE;
        }
      } break;
    }
  }

  return world_data.is_done;
}

void render(State& world_data) {
  SDL_SetRenderDrawColor(world_data.renderer, 255, 255, 255, 0);
  SDL_RenderClear(world_data.renderer);
  for(auto& rend_item : world_data.renderables){
    auto& entity = world_data.entities[rend_item.first];
    auto& rend = rend_item.second;
    ivec2 animation_frame(rend.frames[rend.cur_frame_idx],
                          rend.directions[rend.facing_idx]);
    ivec2 offset = rend.spritesheet_offset + animation_frame;
    SDL_Rect srcrect{offset.x, offset.y, rend.sprite_width, rend.sprite_height};
    SDL_Rect destrect{(int)entity.pos.x, (int)entity.pos.y, rend.display_width, rend.display_height};
    SDL_RenderCopy(world_data.renderer, world_data.spritesheet, &srcrect, &destrect);
  }
  SDL_RenderPresent(world_data.renderer);
}
}

int main(int argc, char** argv) {
  bool done = false;
  State world_data;
  time_point<high_resolution_clock> last_clock_time = high_resolution_clock::now();
  while(!done){
    read_input(world_data);
    auto time = high_resolution_clock::now();
    duration<float> elapsed = time - last_clock_time;
    last_clock_time = time;
    done = update_logic(elapsed.count(), world_data);
    render(world_data);
  }
  cout << "Game over\n";
  return 0;
}

