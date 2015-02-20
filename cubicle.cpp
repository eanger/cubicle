#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <iterator>
#include <limits>
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

#define MOB_SPEED 200.0f
#define MOB_MASS 5.0f
#define MAX_FORCE 20.0f
#define MAX_SEE_AHEAD 35
#define MOB_COLLIDE_SIZE 30
#define DISTANCE_PER_FRAME 20
#define SLOWING_RADIUS 100.0f
#define ARRIVAL_RADIUS 15.0f

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define CHAIR_BUILD_TIME 4.0f
#define CHAIR_PLAN_ALPHA 25

namespace {
random_device rd;

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
  uint8 alpha;
};

struct Task {
  int entity_idx;
  float required_time;
  float time_worked_on;
  enum class Chore {
    IDLE,
    BUILD_CHAIR
  } chore;

  Task()
      : required_time{numeric_limits<float>::infinity()},
        time_worked_on{0},
        chore(Chore::IDLE) {}
  Task(int idx, float t, Chore c)
      : entity_idx{idx}, required_time{t}, time_worked_on{0}, chore{c} {}
};

struct State {
  bool is_done;
  set<Action> actions;
  int next_entity_idx;
  unordered_map<int, Entity> entities;
  unordered_map<int, Renderable> renderables;
  unordered_map<int, Task> workers;
  unordered_set<int> collideables;
  vec2 mouse_pos;
  queue<Task> tasks;
  mt19937 mt;
  uniform_real_distribution<float> rand_x;
  uniform_real_distribution<float> rand_y;

  void delete_entity(int entity) {
    entities.erase(entity);
    renderables.erase(entity);
    workers.erase(entity);
    collideables.erase(entity);
  }

  State()
      : is_done{false},
        next_entity_idx{0},
        mt{rd()},
        rand_x(0, SCREEN_WIDTH),
        rand_y(0, SCREEN_HEIGHT) {}
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
  rend.alpha = 255;
  world_data.collideables.insert(new_chair_idx);
  return new_chair_idx;
}

int make_chair_plan(State& world_data, ivec2 pos) {
  auto idx = make_chair(world_data, pos);
  world_data.renderables[idx].alpha = CHAIR_PLAN_ALPHA;
  world_data.collideables.erase(idx);
  return idx;
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
  rend.alpha = 255;
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

bool update_logic(float dt, State& world_data) {
  for(const auto& action : world_data.actions){
    switch(action){
      case Action::RESET: {
        world_data = State(); 
        return false;
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
        world_data.collideables.insert(new_worker_idx);
      } break;
      case Action::CLICK: {
        // generate new chair task
        auto plan_idx = make_chair_plan(world_data, world_data.mouse_pos);
        world_data.tasks.push({plan_idx, CHAIR_BUILD_TIME, Task::Chore::BUILD_CHAIR});
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
        auto target = world_data.entities[task.entity_idx].pos;
        auto dist = distance(worker.pos, target);
        assert(!isnan(dist) && "invalid distance between worker and target");
        if(dist < ARRIVAL_RADIUS){
          worker.vel = {0,0};
          task.time_worked_on += dt;
          float percent_done = task.time_worked_on / task.required_time;
          if(percent_done >= 1.0f){ 
            make_chair(world_data, target);
            world_data.delete_entity(task.entity_idx);
            task.chore = Task::Chore::IDLE;
          } else {
            auto& rend = world_data.renderables[task.entity_idx];
            rend.alpha = mix(CHAIR_PLAN_ALPHA, 255, percent_done);
          }
        } else {
          auto desired_force = vec2{0};
          if(isNonZero(target - worker.pos)){
            desired_force = normalize(target - worker.pos) * MAX_FORCE;
          }

          // collision avoidance
          if(isNonZero(worker.vel)){
            auto ahead = worker.pos + truncate(worker.vel, MAX_SEE_AHEAD);
            float closest = MAX_SEE_AHEAD;
            vec2 avoid_force(0);
            for(const auto& collide_idx : world_data.collideables){
              const auto& collide = world_data.entities[collide_idx];
              if(collide_idx == id){ continue; }
              if(length(ahead - collide.pos) > MOB_COLLIDE_SIZE){ continue; }
              if(distance(worker.pos, collide.pos) < closest){
                closest = distance(worker.pos, collide.pos);
                avoid_force = ahead - collide.pos;
              }
            }
            desired_force += avoid_force;
          }

          // arrival
          if(dist < SLOWING_RADIUS){
            auto slowing_amount = clamp(dist / SLOWING_RADIUS, 0.1f, 1.0f);
            desired_force *= slowing_amount;
          }

          auto steering_force = truncate(desired_force, MAX_FORCE);
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
        }
      } break;
    }
  }

  return world_data.is_done;
}

void render(State& world_data, SDL_Renderer* renderer,
            SDL_Texture* spritesheet) {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
  SDL_RenderClear(renderer);
  for(const auto& rend_item : world_data.renderables){
    const auto& entity = world_data.entities[rend_item.first];
    const auto& rend = rend_item.second;
    ivec2 animation_frame(rend.frames[rend.cur_frame_idx],
                          rend.directions[rend.facing_idx]);
    ivec2 offset = rend.spritesheet_offset + animation_frame;
    SDL_Rect srcrect{offset.x, offset.y, rend.sprite_width, rend.sprite_height};
    SDL_Rect destrect{(int)entity.pos.x, (int)entity.pos.y, rend.display_width, rend.display_height};
    SDL_SetTextureAlphaMod(spritesheet, rend.alpha);
    SDL_RenderCopy(renderer, spritesheet, &srcrect, &destrect);
    SDL_SetTextureAlphaMod(spritesheet, 255);
  }
  SDL_RenderPresent(renderer);
}
}

int main(int argc, char** argv) {
  SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  auto window = SDL_CreateWindow("Cubicle: Get To Work",
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SCREEN_WIDTH,
                                 SCREEN_HEIGHT,
                                 SDL_WINDOW_OPENGL);
  auto renderer =
      SDL_CreateRenderer(window, -1 /* first available */,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  auto surf = IMG_Load(get_asset_path("spritesheet.bmp").c_str());
  if(!surf){
    cerr << "Bad img load: " << IMG_GetError() << endl;
    exit(-1);
  }
  auto spritesheet = SDL_CreateTextureFromSurface(renderer, surf);
  if(!spritesheet){
    cerr << "Bad convert to texture: " << SDL_GetError() << endl;
    exit(-1);
  }

  bool done = false;
  State world_data;
  time_point<high_resolution_clock> last_clock_time = high_resolution_clock::now();
  while(!done){
    read_input(world_data);
    auto time = high_resolution_clock::now();
    duration<float> elapsed = time - last_clock_time;
    last_clock_time = time;
    done = update_logic(elapsed.count(), world_data);
    render(world_data, renderer, spritesheet);
  }
  cout << "Game over\n";
  return 0;
}

