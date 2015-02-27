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
#define GRID_SQUARE_SIZE 30

#define MAX_SPEED 200.0f
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

#define IDLE_TASK_MIN_TIME 0.0f
#define IDLE_TASK_MAX_TIME 1.0f
#define MAX_WANDER_DIST 10.0f

#define SCROLL_SPEED 20 

namespace {
random_device rd;

enum class Action {
  LEFT,
  RIGHT,
  UP,
  DOWN,
  SHOOT,
  CLICK,
  RESET,
  ZOOM_IN,
  ZOOM_OUT,
  SELECT_CHAIR
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
      : entity_idx{0},
        required_time{numeric_limits<float>::infinity()},
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
  float zoom_ratio;
  vec2 camera;
  int brush_id;

  void delete_entity(int entity) {
    entities.erase(entity);
    renderables.erase(entity);
    workers.erase(entity);
    collideables.erase(entity);
  }

  State()
      : is_done{false},
        next_entity_idx{1},
        zoom_ratio{1.0f},
        camera{0},
        brush_id{0} {}
};

void readInput(State& world_data) {
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
          case SDLK_1: {
            world_data.actions.insert(Action::SELECT_CHAIR);
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
      } break;
      case SDL_MOUSEWHEEL: {
        if(event.wheel.y > 0){
          world_data.actions.insert(Action::ZOOM_IN);
        } else {
          world_data.actions.insert(Action::ZOOM_OUT);
        }
      } break;
      case SDL_MOUSEMOTION: {
        world_data.mouse_pos = vec2{event.motion.x / world_data.zoom_ratio,
                                    event.motion.y / world_data.zoom_ratio} + 
                               world_data.camera;
      } break;
    }
  }
}

int makeChair(State& world_data, ivec2 pos) {
  auto new_chair_idx = world_data.next_entity_idx++;
  auto& new_chair = world_data.entities[new_chair_idx];
  new_chair.pos = pos;
  new_chair.vel = {0,0};
  auto& rend = world_data.renderables[new_chair_idx];
  rend.display_width = GRID_SQUARE_SIZE;
  rend.sprite_width = CHAIR_WIDTH;
  rend.display_height = GRID_SQUARE_SIZE;
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

int makeChairPlan(State& world_data, ivec2 pos) {
  auto idx = makeChair(world_data, pos);
  world_data.renderables[idx].alpha = CHAIR_PLAN_ALPHA;
  world_data.collideables.erase(idx);
  return idx;
}

void makeWorkerRend(Renderable& rend) {
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

float getRandInRange(float start, float end) {
  static random_device rd;
  static mt19937 mt{rd()};
  uniform_real_distribution<float> rand{start, end};
  return rand(mt);
}

void moveTowardsTarget(State& world_data, int worker_idx, vec2& target, float dt) {
  auto& worker = world_data.entities[worker_idx];
  auto desired_vel = vec2{0};
  if(isNonZero(target - worker.pos)){
    desired_vel = normalize(target - worker.pos);
  }

  /*
  if(isNonZero(worker.vel)){
    auto ahead = worker.pos + truncate(worker.vel, MAX_SEE_AHEAD);
    float closest = MAX_SEE_AHEAD;
    vec2 avoid_force(0);
    for(const auto& collide_idx : world_data.collideables){
      const auto& collide = world_data.entities[collide_idx];
      if(collide_idx == worker_idx){ continue; }
      if(length(ahead - collide.pos) > MOB_COLLIDE_SIZE){ continue; }
      if(distance(worker.pos, collide.pos) < closest){
        closest = distance(worker.pos, collide.pos);
        avoid_force = ahead - collide.pos;
      }
    }
    desired_force += avoid_force;
  }
  */

  // arrival
  auto dist = distance(worker.pos, target);
  if(dist < SLOWING_RADIUS){
    auto slowing_amount = mix(0.0f, MAX_SPEED, dist / SLOWING_RADIUS);
    desired_vel *= slowing_amount;
  } else {
    desired_vel *= MAX_SPEED;
  }
  
  auto desired_force = desired_vel - worker.vel;
  auto steering = truncate(desired_force, MAX_FORCE);
  worker.vel += steering / MOB_MASS;

  // down right up left
  float dot_prods[4] = {dot(worker.vel, {0, 1}),  dot(worker.vel, {1, 0}),
                        dot(worker.vel, {0, -1}), dot(worker.vel, {-1, 0})};
  auto max_elem = max_element(begin(dot_prods), end(dot_prods));
  auto facing_idx = std::distance(begin(dot_prods), max_elem);
  if(isNonZero(worker.vel)){
    auto& rend = world_data.renderables[worker_idx];
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

void makeIdleTask(State& world_data, Task& task, Entity& worker) {
  task.chore = Task::Chore::IDLE;
  if(task.entity_idx != 0){
    world_data.entities.erase(task.entity_idx);
  }
  task.entity_idx = world_data.next_entity_idx++;
  auto& new_entity = world_data.entities[task.entity_idx];
  task.time_worked_on = 0;
  task.required_time = getRandInRange(IDLE_TASK_MIN_TIME, IDLE_TASK_MAX_TIME);
  new_entity.pos = vec2{getRandInRange(worker.pos.x - MAX_WANDER_DIST,
                                       worker.pos.x + MAX_WANDER_DIST),
                        getRandInRange(worker.pos.y - MAX_WANDER_DIST,
                                       worker.pos.y + MAX_WANDER_DIST)};
}

bool updateLogic(float dt, State& world_data) {
  for(const auto& action : world_data.actions){
    switch(action){
      case Action::LEFT: {
        world_data.camera.x -= SCROLL_SPEED;
      } break;
      case Action::RIGHT: {
        world_data.camera.x += SCROLL_SPEED;
      } break;
      case Action::UP: {
        world_data.camera.y -= SCROLL_SPEED;
      } break;
      case Action::DOWN: {
        world_data.camera.y += SCROLL_SPEED;
      } break;
      case Action::RESET: {
        world_data = State(); 
        return false;
      } break; 
      case Action::SHOOT: {
        // generate new worker
        auto new_worker_idx = world_data.next_entity_idx++;
        auto& new_worker = world_data.entities[new_worker_idx];
        new_worker.pos = {getRandInRange(0, SCREEN_WIDTH),
                          getRandInRange(0, SCREEN_HEIGHT)};
        new_worker.vel = {0,0};
        auto& new_worker_rend = world_data.renderables[new_worker_idx];
        makeWorkerRend(new_worker_rend);
        world_data.workers[new_worker_idx];
        world_data.collideables.insert(new_worker_idx);
      } break;
      case Action::CLICK: {
        // generate new chair task
        if(world_data.brush_id != 0){
          auto chair_pos = world_data.mouse_pos;
          chair_pos.x -= GRID_SQUARE_SIZE / 2;
          chair_pos.y -= GRID_SQUARE_SIZE / 2;
          world_data.tasks.push({world_data.brush_id, CHAIR_BUILD_TIME, Task::Chore::BUILD_CHAIR});
          world_data.brush_id = makeChairPlan(world_data, chair_pos);
        }
      } break;
      case Action::ZOOM_IN: {
        world_data.zoom_ratio *= 1.25f;
      } break;
      case Action::ZOOM_OUT: {
        world_data.zoom_ratio /= 1.25f;
      } break;
      case Action::SELECT_CHAIR: {
        if(world_data.brush_id == 0){
          world_data.brush_id =  makeChairPlan(world_data, world_data.mouse_pos);
        } else { 
          world_data.delete_entity(world_data.brush_id);
          world_data.brush_id = 0;
        }
      } break;
    }
  }
  world_data.actions.clear();

  // update brush position
  if(world_data.brush_id != 0){
    auto& ent = world_data.entities[world_data.brush_id];
    ent.pos = world_data.mouse_pos - mod(world_data.mouse_pos, (float) GRID_SQUARE_SIZE);
  }

  for(auto& worker_iter : world_data.workers){
    const auto& id = worker_iter.first;
    auto& task = worker_iter.second;
    auto& worker = world_data.entities[id];
    auto target = world_data.entities[task.entity_idx].pos;
    task.time_worked_on += dt;
    float percent_done = task.time_worked_on / task.required_time;
    switch(task.chore){
      case Task::Chore::IDLE: {
        if(!world_data.tasks.empty()){
          task = world_data.tasks.front();
          world_data.tasks.pop();
        }
        if(task.entity_idx != 0 && percent_done < 1.0f){
          moveTowardsTarget(world_data, id, target, dt);
        } else {
          // if we've surpassed our bounds, make a new IDLE task with a random duration
          makeIdleTask(world_data, task, worker);
        }

      } break;
      case Task::Chore::BUILD_CHAIR: {
        auto dist = distance(worker.pos, target);
        assert(!isnan(dist) && "invalid distance between worker and target");
        if(dist < ARRIVAL_RADIUS){
          worker.vel = {0,0};
          if(percent_done >= 1.0f){ 
            makeChair(world_data, target);
            world_data.delete_entity(task.entity_idx);
            makeIdleTask(world_data, task, worker);
          } else {
            auto& rend = world_data.renderables[task.entity_idx];
            rend.alpha = mix(CHAIR_PLAN_ALPHA, 255, percent_done);
          }
        } else {
          moveTowardsTarget(world_data, id, target, dt);
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
    auto screen_pos = (entity.pos - world_data.camera) * world_data.zoom_ratio;
    SDL_Rect destrect{(int)screen_pos.x,
                      (int)screen_pos.y,
                      (int)(rend.display_width * world_data.zoom_ratio),
                      (int)(rend.display_height * world_data.zoom_ratio)};
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
    readInput(world_data);
    auto time = high_resolution_clock::now();
    duration<float> elapsed = time - last_clock_time;
    last_clock_time = time;
    done = updateLogic(elapsed.count(), world_data);
    render(world_data, renderer, spritesheet);
  }
  cout << "Game over\n";
  return 0;
}

