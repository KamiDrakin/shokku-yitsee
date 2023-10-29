#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "raylib.h"
#include "raymath.h"

#include "datstructs.h"
#include "symath.h"
#include "models.h"
#include "maps.h"

#ifdef PLATFORM_WEB
  #include <emscripten/emscripten.h>
#endif

#ifdef PLATFORM_DESKTOP
  #define GLSL_VERSION            330
#else //PLATFORM_ANDROID, PLATFORM_WEB
  #define GLSL_VERSION            100
#endif

#define DIVINE 1.618f
#define INV_DIVINE 0.618f
#define ALMOST_ZERO 0.0001f

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(v, b, t) ((v) < (b) ? (b) : (v) > (t) ? (t) : (v))
#define BETWEEN(v, b, t) ((v) >= (b) && (v) <= (t))

#define INITIAL_SCREEN_W 1024
#define INITIAL_SCREEN_H 768

#define GAME_W 256
#define GAME_H 192

#define FPS 0
#define TPS 2.0f

#define COLOR_DEPTH_R 16
#define COLOR_DEPTH_G 16
#define COLOR_DEPTH_B 16

#define MAX_TOUCH_POINTS 10

#define BASE_CAM_DIST 256.0f
#define CAM_FOLLOW_SPEED 3.0f
#define CAM_VERT_OFFSET 1.0f

#define TILE_SIZE 16.0f
#define CHUNK_SIZE 16
#define CHUNK_SIZE_S (CHUNK_SIZE * CHUNK_SIZE)
#define CHUNK_HEIGHT_CAP 10000.0f
#define MAX_ACTIVE_CHUNKS 41

#define MAX_NAME_LENGTH 20
#define MAX_FACINGS 8
#define MAX_ANIMATIONS 16
#define STEP_SNAP_HEIGHT 0.5f
#define GRAVITY 20.0f

#define MAX_USABLE_ITEMS 4
#define MAX_COMMANDS 4

#define MAX_PARTY_SIZE 4

#define MAX_ACTIVE_NPCS 256
#define MAX_INACTIVE_NPCS 2048

typedef struct Basic3D Basic3D;
typedef struct Basic2D Basic2D;
typedef struct Input Input;
typedef struct CamPoint CamPoint;
typedef struct Pusher Pusher;
typedef struct WorldChunk WorldChunk;
typedef struct StaticObject StaticObject;
typedef struct Animation Animation;
typedef struct GameObject GameObject;
typedef struct CombatObject CombatObject;
typedef struct PCObject PCObject;
typedef struct PlayerObject PlayerObject;
typedef struct NPCObject NPCObject;
typedef struct ObjectKeeper ObjectKeeper;

typedef enum {
  CARDINAL_NORTH = 0,
  CARDINAL_EAST,
  CARDINAL_SOUTH,
  CARDINAL_WEST
} Cardinals;

struct Basic3D {
  Shader shader;
  int light_src_loc;
  int color_depth_loc;
  int light_intensity_loc;
  int with_texture_loc;
};

struct Basic2D {
  Shader shader;
  int color_depth_loc;
};

struct Input {
  float move_speed;
  Vector3 move_translate;
  float cam_rotate;
  float cam_rotate_v;
  float zoom_factor;
};

struct CamPoint {
  Camera3D cam;
  float dist;
  float rot_pi;
  float rot_v_pi;
  float cos_rot_v;
  float zoom;
  Vector3 left;
  Vector3 forward;
  //Vector3 rel_up;
  GameObject *follow_obj;
};

struct Pusher {
  Vector2 v1;
  Vector2 v2;
  Vector2 normal;
  Vector2 p;
  float h;
};

struct WorldChunk {
  float height_map[CHUNK_SIZE_S];
  float max_height;
  float min_height;
  int w_pos[2];
  WorldChunk *neighbours[4]; //NESW
  Mesh mesh;
  Model model;
  Color tint;
};

struct StaticObject {
  WorldChunk *current_chunk;
  Vector3 pos;
  Mesh mesh;
  Model model;
};

struct Animation {
  float frame_index; //float for smooth stop/start
  Texture2D texture;
};

struct GameObject {
  char name[MAX_NAME_LENGTH];
  WorldChunk *current_chunk;
  Vector3 pos;
  float radius;
  float g_speed;
  Vector3 last_move_dir; //for calculating facing
  int c_facings;
  Vector3 facings[MAX_FACINGS];
  int sprite_size[2]; //each row animation, each col facings
  int c_animations;
  int animation_index;
  Animation animations[MAX_ANIMATIONS];
  Color tint;
  void (*update)(GameObject *);
};

struct CombatObject {
  float base_hp, hp, max_hp;
  float base_mp, mp, max_mp;
  float base_ap, ap, max_ap;
  float base_ep, ep, max_ep;
  int level, sh_tnl;
  int base_atk, atk;
  int base_acc, acc;
  int base_mag, mag;
  int base_def, def;
  int base_mdf, mdf;
  int base_spd, spd;
  GameObject game_obj;
  void (*update)(CombatObject *);
};

struct PCObject {
  int equipment[5]; //mh, oh, head, body, legs
  int usable_items[MAX_USABLE_ITEMS];
  int commands[MAX_COMMANDS];
  Texture2D portrait;
  CombatObject combat_obj;
  void (*update)(PCObject *);
};

struct PlayerObject {
  int sh, benk_sh;
  int party_size;
  PCObject party[MAX_PARTY_SIZE];
  void (*update)(PlayerObject *);
};

struct NPCObject {
  
  CombatObject combat_obj;
  void (*update)(NPCObject *);
  int (*dialogue)(NPCObject *, int);
  void (*combat_ai)(NPCObject *);
};

struct ObjectKeeper {
  PlayerObject player;
  UQueue active_npcs;
  UQueue inactive_npcs;
};

Color color_d(unsigned char r, unsigned char g, unsigned char b, unsigned char a); //Returns color with applied depth.
float *generate_chunk_vertices(WorldChunk *chunk, int *c_vertices); //Generates vertices from a WorldChunk height map.
WorldChunk *get_chunk_at(WorldChunk *origin, Vector2 pos); //Returns a neighbouring chunk if given position is out of bounds.
float get_chunk_height_at(WorldChunk *chunk, Vector2 pos); //Returns the y coordinate of WorldChunk's height map at (x, z).
void join_chunks(WorldChunk *chunk1, Cardinals cardinal, WorldChunk *chunk2); //Set chunks as neighbours and assign world position to chunk2.
void calculate_normals(float *normals, const float *vertices, int c_vertices); //Calculates normals for each triangle.
Mesh generate_mesh(const float *vertices, int c_vertices); //Generates a custom Mesh (all vertices WHITE).
void setup(); //Sets up the game.
void process_keyboard(); //Processes keyboard inputs.
void process_mouse(); //Processes mouse inputs.
void process_controller(); //Processes controller inputs.
void process_touch(); //Processes touch inputs.
void cam_point_update(Vector3 translate, float rotate, float rotate_v, float zoom_factor); //Translates camera target and rotates camera position.
void move_game_object(GameObject *obj, Vector2 v); //Move game object and update its movement-related properties.
void draw_background(); //Draws a basic background.
Rectangle get_game_object_frame(GameObject *obj); //Get the current animation/facing frame of an object.
void draw_game_object(GameObject *obj);
void update_draw(); //Update and draw.

bool fullscreen = false;
float delta;
float screen_scale;

RenderTexture2D render_target;

Basic3D basic3d = {0};
Basic2D basic2d = {0};

Input input = {0};

CamPoint cam_point = {0};

WorldChunk test_chunks[64] = {0};

Vector2 prev_touch_points[MAX_TOUCH_POINTS];

bool light_switch = false;

UQueue active_chunks;

ObjectKeeper object_keeper;
GameObject test_object = {0};

float turn_keeper = 0.0f;
bool next_turn;

Color color_d(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  float ratio_r = (float)r / 0xff;
  float ratio_g = (float)g / 0xff;
  float ratio_b = (float)b / 0xff;
  return (Color){
    round(ratio_r * COLOR_DEPTH_R) / COLOR_DEPTH_R * 0xff,
    round(ratio_g * COLOR_DEPTH_G) / COLOR_DEPTH_G * 0xff,
    round(ratio_b * COLOR_DEPTH_B) / COLOR_DEPTH_B * 0xff,
    a
  };
}

float *generate_chunk_vertices(WorldChunk *chunk, int *c_vertices) {
  const int c_square_vertices = 18;
  int c_h_vertices = CHUNK_SIZE_S * c_square_vertices;
  int c_v_vertices = c_h_vertices * 2; // - CHUNK_SIZE * 2 * c_square_vertices;
  float *vertices = malloc((c_h_vertices + c_v_vertices) * sizeof(float));
  
  int p = 0;
  for (int i = 0; i < CHUNK_SIZE_S; i++) { //each square
    int pos_x = i % CHUNK_SIZE;
    int pos_z = i / CHUNK_SIZE;
    float tile_x = pos_x;
    float tile_z = pos_z;
    
    float h_height;
    
    //horizontal squares
    if (chunk->height_map[i] <= chunk->max_height)
      h_height = chunk->height_map[i];
    else
      h_height = CHUNK_HEIGHT_CAP;
    const float h_square[] = {
      tile_x, h_height, tile_z,               //0  z
      tile_x, h_height, tile_z + 1.0f,        //1  : 1 5--3
      tile_x + 1.0f, h_height, tile_z,        //2  : |\ \ |
      tile_x + 1.0f, h_height, tile_z + 1.0f, //3  : | \ \|
      tile_x + 1.0f, h_height, tile_z,        //4  : 0--2 4
      tile_x, h_height, tile_z + 1.0f         //5  o.......x
    };
    memcpy(vertices + p, h_square, c_square_vertices * sizeof(float));
    p += c_square_vertices;
    
    //vertical rects
    if (chunk->height_map[i] <= chunk->max_height)
      h_height = chunk->height_map[i];
    else
      h_height = chunk->max_height;
    float l_y;
    if (i % CHUNK_SIZE > 0)
      l_y = chunk->height_map[i - 1];
    else if (chunk->neighbours[3] != NULL)
      l_y = chunk->neighbours[3]->height_map[i + CHUNK_SIZE - 1];
    else
      l_y = h_height;
    if (l_y > chunk->max_height)
      l_y = chunk->max_height;
    const float l_square[] = {
      h_square[0 * 3 + 0], l_y, h_square[0 * 3 + 2], //left 4
      h_square[1 * 3 + 0], l_y, h_square[1 * 3 + 2], //left 3
      h_square[1 * 3 + 0], h_height, h_square[1 * 3 + 2], //this 1
      h_square[1 * 3 + 0], h_height, h_square[1 * 3 + 2], //this 1
      h_square[0 * 3 + 0], h_height, h_square[0 * 3 + 2], //this 0
      h_square[0 * 3 + 0], l_y, h_square[0 * 3 + 2] //left 4
    };
    memcpy(vertices + p, l_square, c_square_vertices * sizeof(float));
    p += c_square_vertices;
    
    float b_y;
    if (i / CHUNK_SIZE > 0)
      b_y = chunk->height_map[i - CHUNK_SIZE];
    else if (chunk->neighbours[0] != NULL)
      b_y = chunk->neighbours[0]->height_map[i + CHUNK_SIZE_S - CHUNK_SIZE];
    else
      b_y = h_height;
    if (b_y > chunk->max_height)
      b_y = chunk->max_height;
    const float b_square[] = {
      h_square[2 * 3 + 0], b_y, h_square[2 * 3 + 2], //back 3
      h_square[0 * 3 + 0], b_y, h_square[0 * 3 + 2], //back 5
      h_square[0 * 3 + 0], h_height, h_square[0 * 3 + 2], //this 0
      h_square[2 * 3 + 0], h_height, h_square[2 * 3 + 2], //this 2
      h_square[2 * 3 + 0], b_y, h_square[2 * 3 + 2], //back 3
      h_square[0 * 3 + 0], h_height, h_square[0 * 3 + 2]  //this 0
    };
    memcpy(vertices + p, b_square, c_square_vertices * sizeof(float));
    p += c_square_vertices;
  }
  *c_vertices = c_h_vertices + c_v_vertices;
  return vertices;
}

WorldChunk *get_chunk_at(WorldChunk *origin, Vector2 pos) {
  int diff_x = floor(pos.x / CHUNK_SIZE) - origin->w_pos[0];
  int diff_z = floor(pos.y / CHUNK_SIZE) - origin->w_pos[1];
  if (diff_z != 0) {
    WorldChunk *neighbour = origin->neighbours[diff_z == 1 ? CARDINAL_SOUTH : CARDINAL_NORTH];
    if (neighbour == NULL)
      return NULL;
    origin = neighbour;
  }
  if (diff_x != 0) {
    WorldChunk *neighbour = origin->neighbours[diff_x == 1 ? CARDINAL_EAST : CARDINAL_WEST];
    if (neighbour == NULL)
      return NULL;
    origin = neighbour;
  }
  return origin;
}

float get_chunk_height_at(WorldChunk *chunk, Vector2 pos) {
  int i_x = (int)pos.x % CHUNK_SIZE;
  if (i_x < 0)
    i_x += CHUNK_SIZE;
  int i_z = (int)pos.y % CHUNK_SIZE;
  if (i_z < 0)
    i_z += CHUNK_SIZE;
  return chunk->height_map[i_z * CHUNK_SIZE + i_x];
}

void join_chunks(WorldChunk *chunk1, Cardinals cardinal, WorldChunk *chunk2) {
  const int w_x[] = {0, 1, 0, -1};
  const int w_z[] = {-1, 0, 1, 0};
  cardinal = CLAMP(cardinal, CARDINAL_NORTH, CARDINAL_WEST);
  chunk1->neighbours[cardinal] = chunk2;
  chunk2->neighbours[(cardinal + 2) % 4] = chunk1;
  chunk2->w_pos[0] = chunk1->w_pos[0] + w_x[cardinal];
  chunk2->w_pos[1] = chunk1->w_pos[1] + w_z[cardinal];
}

void calculate_normals(float *normals, const float *vertices, int c_vertices) {
  for (int i = 0; i < c_vertices / 9; i++) {
    Vector3 *v = (Vector3 *)vertices + i * 3 + 0;
    Vector3 *v1 = (Vector3 *)vertices + i * 3 + 1;
    Vector3 *v2 = (Vector3 *)vertices + i * 3 + 2;
    Vector3 normal = Vector3CrossProduct(Vector3Subtract(*v, *v1), Vector3Subtract(*v, *v2));
    for (int j = 0; j < 3; j++) {
      memcpy(normals + i * 9 + j * 3, &normal, 3 * sizeof(float));
    }
  }
}

Mesh generate_mesh(const float *vertices, int c_vertices) {
  Mesh mesh = {0};
  mesh.vertexCount = c_vertices / 3;
  mesh.triangleCount = c_vertices / 9;
  mesh.vertices = malloc(c_vertices * sizeof(float));
  //mesh.texcoords = (float *)malloc(mesh.vertexCount * 2 * sizeof(float));
  mesh.normals  = malloc(c_vertices * sizeof(float));
  mesh.colors   = malloc(c_vertices * 4 / 3 * sizeof(unsigned char));
  
  memcpy(mesh.vertices, vertices, c_vertices * sizeof(float));
  calculate_normals(mesh.normals, vertices, c_vertices);
  for (int i = 0; i < c_vertices / 3; i++) {
    unsigned char c[] = {0xff, 0xff, 0xff, 0xff};
    memcpy(mesh.colors + i * 4, c, 4 * sizeof(unsigned char));
  }
  
  UploadMesh(&mesh, false);
  return mesh;
}

void setup() {
  basic3d.shader = LoadShader(TextFormat("./res/shaders/%i_basic3d.vs", GLSL_VERSION), TextFormat("./res/shaders/%i_basic3d.fs", GLSL_VERSION));
  basic3d.light_src_loc = GetShaderLocation(basic3d.shader, "light_src");
  basic3d.color_depth_loc = GetShaderLocation(basic3d.shader, "color_depth");
  basic3d.light_intensity_loc = GetShaderLocation(basic3d.shader, "light_intensity");
  basic3d.with_texture_loc = GetShaderLocation(basic3d.shader, "with_texture");
  
  SetShaderValue(basic3d.shader, basic3d.color_depth_loc, (float[3]){COLOR_DEPTH_R, COLOR_DEPTH_G, COLOR_DEPTH_B}, SHADER_UNIFORM_VEC3);
  SetShaderValue(basic3d.shader, basic3d.light_intensity_loc, (float[1]){12000.0f}, SHADER_UNIFORM_FLOAT);
  
  basic2d.shader = LoadShader(0, TextFormat("./res/shaders/%i_basic2d.fs", GLSL_VERSION));
  basic2d.color_depth_loc = GetShaderLocation(basic2d.shader, "color_depth");
  
  SetShaderValue(basic2d.shader, basic2d.color_depth_loc, (float[3]){COLOR_DEPTH_R, COLOR_DEPTH_G, COLOR_DEPTH_B}, SHADER_UNIFORM_VEC3);
  
  cam_point.zoom = TILE_SIZE;
  cam_point.cam.position   = Vector3Zero();
  cam_point.cam.target     = Vector3Zero();
  cam_point.cam.up         = (Vector3){0.0f, 1.0f, 0.0f};
  cam_point.cam.fovy       = GAME_H * cam_point.zoom;
  cam_point.cam.projection = CAMERA_ORTHOGRAPHIC;
  cam_point.dist = BASE_CAM_DIST / cam_point.zoom;
  cam_point.rot_v_pi = 1.0f / 6;
  cam_point.rot_pi = 0.25f;
  
  for (int i = 0; i < 64; i++) {
    WorldChunk *chunk = test_chunks + i;
    chunk->max_height = 5.0f;
    for (int j = 0; j < CHUNK_SIZE_S; j++)
      chunk->height_map[j] = rand() * 2.0f / (float)RAND_MAX;
    int x = i % 8;
    int y = i / 8;
    if (y > 0)
      join_chunks(chunk, CARDINAL_NORTH, test_chunks + i - 8);
    if (x < 7)
      join_chunks(chunk, CARDINAL_EAST, test_chunks + i + 1);
    if (y < 7)
      join_chunks(chunk, CARDINAL_SOUTH, test_chunks + i + 8);
    if (x > 0)
      join_chunks(chunk, CARDINAL_WEST, test_chunks + i - 1);
  }
  memcpy(test_chunks, &test_0_0_height_map, CHUNK_SIZE_S * sizeof(float));
  for (int i = 0; i < 64; i++) {
    WorldChunk *chunk = test_chunks + i;
    int c_test_vertices;
    float *test_vertices;
    test_vertices = generate_chunk_vertices(chunk, &c_test_vertices);
    chunk->mesh = generate_mesh(test_vertices, c_test_vertices);
    chunk->model = LoadModelFromMesh(chunk->mesh);
    chunk->model.materials[0].shader = basic3d.shader;
    free(test_vertices);
    chunk->tint = color_d(rand() % 256, rand() % 256, rand() % 256, 0xff);
  }
  active_chunks = uqueue_create(MAX_ACTIVE_CHUNKS, sizeof(WorldChunk *));
  
  object_keeper.active_npcs = uqueue_create(MAX_ACTIVE_NPCS, sizeof(NPCObject *));
  object_keeper.inactive_npcs = uqueue_create(MAX_INACTIVE_NPCS, sizeof(NPCObject *));
  
  test_object.current_chunk = test_chunks;
  test_object.pos = (Vector3){7.5f, 16.0f, 7.5f};
  test_object.radius = 0.25f;
  test_object.sprite_size[0] = 16;
  test_object.sprite_size[1] = 24;
  test_object.c_facings = 8;
  test_object.facings[0] = (Vector3){0.0f, 0.0f, 1.0f};
  test_object.facings[1] = Vector3Normalize((Vector3){1.0f, 0.0f, 1.0f});
  test_object.facings[2] = (Vector3){1.0f, 0.0f, 0.0f};
  test_object.facings[3] = Vector3Normalize((Vector3){1.0f, 0.0f, -1.0f});
  test_object.facings[4] = (Vector3){0.0f, 0.0f, -1.0f};
  test_object.facings[5] = Vector3Normalize((Vector3){-1.0f, 0.0f, -1.0f});
  test_object.facings[6] = (Vector3){-1.0f, 0.0f, 0.0f};
  test_object.facings[7] = Vector3Normalize((Vector3){-1.0f, 0.0f, 1.0f});
  test_object.c_animations = 2;
  Image test_image;
  test_image = LoadImage("./res/textures/purp.png");
  test_object.animations[0].frame_index = 0;
  test_object.animations[0].texture = LoadTextureFromImage(test_image);
  UnloadImage(test_image);
  test_image = LoadImage("./res/textures/purp_jet.png");
  test_object.animations[1].frame_index = 0;
  test_object.animations[1].texture = LoadTextureFromImage(test_image);
  UnloadImage(test_image);
  test_object.tint = (Color){0xff, 0xff, 0xff, 0xff};
  
  cam_point.follow_obj = &test_object;
}

void process_keyboard() {
  Vector3 cam_forward_xz = Vector3Normalize((Vector3){cam_point.forward.x, 0.0f, cam_point.forward.z});
  if (IsKeyDown(KEY_LEFT_SHIFT))
    input.move_speed *= 10.0f;
  if (IsKeyDown(KEY_W))
    input.move_translate = Vector3Add(input.move_translate, cam_forward_xz);
  if (IsKeyDown(KEY_S))
    input.move_translate = Vector3Subtract(input.move_translate, cam_forward_xz);
  if (IsKeyDown(KEY_A))
    input.move_translate = Vector3Add(input.move_translate, cam_point.left);
  if (IsKeyDown(KEY_D))
    input.move_translate = Vector3Subtract(input.move_translate, cam_point.left);
  if (IsKeyDown(KEY_Q))
    input.cam_rotate += 0.2f;
  if (IsKeyDown(KEY_E))
    input.cam_rotate -= 0.2f;
  if (IsKeyDown(KEY_R))
    input.cam_rotate_v -= 0.1f;
  if (IsKeyDown(KEY_F))
    input.cam_rotate_v += 0.1f;
  input.move_translate = Vector3Scale(Vector3Normalize(input.move_translate), input.move_speed);
  
  if (IsKeyDown(KEY_T))
    input.zoom_factor += 0.3f;
  if (IsKeyDown(KEY_G))
    input.zoom_factor -= 0.3f;
  
  if (IsKeyPressed(KEY_L)) {
    SetShaderValue(basic3d.shader, basic3d.light_intensity_loc, (float[1]){light_switch ? 12000.0f : INV_DIVINE * 10.0f}, SHADER_UNIFORM_FLOAT);
    light_switch = !light_switch;
  }
  
  if (IsKeyDown(KEY_SPACE)) {
    test_object.g_speed -= 30.0f * delta;
    test_object.animation_index = 1;
    test_object.animations[test_object.animation_index].frame_index += 10.0f * delta;
  }
  else
    test_object.animation_index = 0;
  
#ifdef PLATFORM_DESKTOP
  if (IsKeyPressed(KEY_F11)) {
    if (fullscreen) {
      ClearWindowState(FLAG_FULLSCREEN_MODE);
      SetWindowSize(INITIAL_SCREEN_W, INITIAL_SCREEN_H);
    }
    else {
      int monitor = GetCurrentMonitor();
      SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
      SetWindowState(FLAG_FULLSCREEN_MODE);
    }
    fullscreen = !fullscreen;
  }
#endif
}

void process_mouse() {
  //soon(tm)
}

void process_controller() {
  //soon
}

void process_touch() {
  //barely works
  /*
  Vector3 cam_forward_xz = Vector3Normalize((Vector3){cam_point.forward.x, 0.0f, cam_point.forward.z});
  Vector2 raw_touch_points[MAX_TOUCH_POINTS];
  Vector2 touch_points[MAX_TOUCH_POINTS];
  Vector2 touch_diffs[MAX_TOUCH_POINTS] = {0};
  int c_touch_points = CLAMP(GetTouchPointCount(), 0, MAX_TOUCH_POINTS);
  if (c_touch_points == 1 && !(IsGestureDetected(GESTURE_HOLD) || IsGestureDetected(GESTURE_DRAG)))
    c_touch_points = 0;
  for (int i = 0; i < c_touch_points; i++) {
    raw_touch_points[i] = GetTouchPosition(i);
    touch_points[i].x = (raw_touch_points[i].x - (GetScreenWidth() - (GAME_W * screen_scale)) * 0.5f) / screen_scale;
    touch_points[i].y = (raw_touch_points[i].y - (GetScreenHeight() - (GAME_H * screen_scale)) * 0.5f) / screen_scale;
    if (prev_touch_points[i].x > 0.0f || prev_touch_points[i].y > 0.0f)
      touch_diffs[i] = Vector2Subtract(touch_points[i], prev_touch_points[i]);
  }
  for (int i = 0; i < MAX_TOUCH_POINTS; i++)
    prev_touch_points[i] = (Vector2){0.0f, 0.0f};
  memcpy(prev_touch_points, touch_points, c_touch_points * sizeof(Vector2));
  
  if (c_touch_points == 2 && IsGestureDetected(GESTURE_HOLD) && BETWEEN(GetGestureHoldDuration(), 5.0f, 5.0f + delta)) {
    SetShaderValue(basic3d.shader, basic3d.light_intensity_loc, (float[1]){light_switch ? 12000.0f : 100.0f}, SHADER_UNIFORM_FLOAT);
    light_switch = !light_switch;
  }
  else if (c_touch_points == 1 && IsGestureDetected(GESTURE_HOLD) && BETWEEN(GetGestureHoldDuration(), 5.0f, 5.0f + delta)) {
    for (int i = 0; i < CHUNK_SIZE_S; i++) {
      if (BETWEEN(i % CHUNK_SIZE, 1, CHUNK_SIZE - 2) && BETWEEN(i / CHUNK_SIZE, 1, CHUNK_SIZE - 2))
        test_chunk.height_map[i] = rand() * 32.0f / (float)RAND_MAX;
    }
    int c_test_vertices;
    float *test_vertices = generate_chunk_vertices(&test_chunk, &c_test_vertices);
    float *test_normals = malloc(c_test_vertices * sizeof(float));
    calculate_normals(test_normals, test_vertices, c_test_vertices);
    UpdateMeshBuffer(test_chunk.mesh, 0, test_vertices, c_test_vertices * sizeof(float), 0);
    UpdateMeshBuffer(test_chunk.mesh, 2, test_normals, c_test_vertices * sizeof(float), 0);
    free(test_vertices);
    free(test_normals);
  }
  else if (c_touch_points == 2) {
    input.cam_rotate = -(touch_diffs[1].x + touch_diffs[0].x) / 2 * 0.1f;
    input.cam_rotate_v = (touch_diffs[1].y + touch_diffs[0].y) / 2 * 0.05f;
  }
  else if (c_touch_points == 1 && (touch_diffs[0].x != 0.0f || touch_diffs[0].y != 0.0f)) {
    input.move_translate = (Vector3){0.0f, 0.0f, 0.0f};
    input.move_translate = Vector3Add(input.move_translate, Vector3Scale(cam_point.left, touch_diffs[0].x));
    input.move_translate = Vector3Add(input.move_translate, Vector3Scale(cam_forward_xz, touch_diffs[0].y));
    input.move_translate = Vector3Scale(input.move_translate, 2.0f);
  }
  */
}

void cam_point_update(Vector3 translate, float rotate, float rotate_v, float zoom_factor) {
  if (cam_point.follow_obj == NULL)
    cam_point.cam.target = Vector3Add(cam_point.cam.target, translate);
  else {
    Vector3 dir = Vector3Subtract(Vector3Add(cam_point.follow_obj->pos, (Vector3){0.0f, CAM_VERT_OFFSET, 0.0f}), cam_point.cam.target);
    float dist = Vector3Length(dir);
    dist = CLAMP(dist * CAM_FOLLOW_SPEED * delta, 0.0f, dist);
    dir = Vector3Normalize(dir);
    cam_point.cam.target = Vector3Add(cam_point.cam.target, Vector3Scale(dir, dist));
  }
  cam_point.rot_pi += rotate;
  cam_point.rot_v_pi += rotate_v;
  cam_point.zoom *= 1.0f + zoom_factor;
  if (cam_point.rot_pi > 1.0f)
    cam_point.rot_pi -= 2.0f;
  else if (cam_point.rot_pi < -1.0f)
    cam_point.rot_pi += 2.0f;
  cam_point.rot_v_pi = CLAMP(cam_point.rot_v_pi, 1.0f / 8, 1.0f / 4);
  cam_point.cos_rot_v = cos(cam_point.rot_v_pi * PI);
  Vector3 norm_pos = Vector3Normalize((Vector3){
    sin(cam_point.rot_pi * PI) * cam_point.dist,
    tan(cam_point.rot_v_pi * PI) * cam_point.dist,
    cos(cam_point.rot_pi * PI) * cam_point.dist
  });
  cam_point.dist = BASE_CAM_DIST / cam_point.zoom;
  cam_point.cam.position = Vector3Add(cam_point.cam.target, Vector3Scale(norm_pos, cam_point.dist));
  cam_point.cam.fovy = GAME_H / cam_point.zoom;
  cam_point.forward = Vector3Normalize(Vector3Subtract(cam_point.cam.target, cam_point.cam.position));
  cam_point.left = Vector3Normalize(Vector3CrossProduct(cam_point.cam.up, cam_point.forward));
  //cam_point.rel_up = Vector3Normalize(Vector3CrossProduct(cam_point.forward, cam_point.left)); //for billboard up vector
}

void move_game_object(GameObject *obj, Vector2 v) {
  WorldChunk *new_chunk;
  Vector2 new_pos = vector3_xz(obj->pos);
  float pos_y = obj->pos.y;
  Vector2 remaining = Vector2Zero();
  if (!Vector2Equals(v, Vector2Zero())) {
    if (Vector2Length(v) >= obj->radius) {
      remaining = v;
      v = Vector2Scale(Vector2Normalize(v), obj->radius - ALMOST_ZERO); //can't be arsed to do logic for more than 1 radius per frame
      remaining = Vector2Subtract(remaining, v);
    }
    new_pos = Vector2Add(new_pos, v);
    obj->last_move_dir = vector2_to_xz(Vector2Normalize(v), 0.0f);
  }
  
  const int w_x[] = {0, 1, 0, -1};
  const int w_y[] = {-1, 0, 1, 0};
  const int p_x[] = {0, 1, 1, 0};
  const int p_y[] = {0, 0, 1, 1};
  
  BTree pushers = btree_create(sizeof(Pusher));
  UQueue lifters = uqueue_create(4 * 9, sizeof(Pusher));
  Pusher pusher;
  
  for (float y = -1.0f; y <= 1.0f; y += 1.0f) {
    for (float x = -1.0f; x <= 1.0f; x += 1.0f)  {
      Vector2 trans_pos = Vector2Add(new_pos, (Vector2){x, y});
      new_chunk = get_chunk_at(obj->current_chunk, trans_pos);
      float h = new_chunk != NULL ? get_chunk_height_at(new_chunk, trans_pos) : CHUNK_HEIGHT_CAP;
      for (int i = 0; i < 4; i++) {
        pusher.v1.x = x + floor(new_pos.x) + p_x[i];
        pusher.v1.y = y + floor(new_pos.y) + p_y[i];
        pusher.v2.x = x + floor(new_pos.x) + p_x[(i + 1) % 4];
        pusher.v2.y = y + floor(new_pos.y) + p_y[(i + 1) % 4];
        pusher.normal = (Vector2){w_x[i], w_y[i]};
        pusher.p = closest_point_on_line(pusher.v1, pusher.v2, new_pos);
        pusher.h = h;
        btree_push(&pushers, Vector2LengthSqr(Vector2Subtract(pusher.p, new_pos)), &pusher);
      }
    }
  }
  while (btree_pop_low(&pushers, &pusher)) {
    pusher.p = closest_point_on_line(pusher.v1, pusher.v2, new_pos);
    Vector2 c = unclipping_vector(new_pos, obj->radius, pusher.p, pusher.normal);
    if (Vector2LengthSqr(c) == 0.0f)
      continue;
    if (pos_y + STEP_SNAP_HEIGHT < pusher.h)
      new_pos = Vector2Add(new_pos, c);
    else
      uqueue_push(&lifters, &pusher);
  }
  
  new_chunk = get_chunk_at(obj->current_chunk, new_pos);
  if (new_chunk == NULL)
    return;
  obj->current_chunk = new_chunk;
  obj->pos = vector2_to_xz(new_pos, pos_y);
  
  float highest_point = get_chunk_height_at(obj->current_chunk, new_pos);
  while (uqueue_pop(&lifters, &pusher)) {
    pusher.p = closest_point_on_line(pusher.v1, pusher.v2, new_pos);
    float dist_sqr = Vector2LengthSqr(Vector2Subtract(pusher.p, new_pos));
    if (dist_sqr >= pow(obj->radius, 2))
      continue;
    if (highest_point < pusher.h)
      highest_point = pusher.h;
  }
  btree_destroy(&pushers);
  uqueue_destroy(&lifters);
  
  bool repeat = !Vector2Equals(remaining, Vector2Zero());
  
  if (!repeat) {
    if (highest_point < pos_y)
      obj->g_speed += GRAVITY * delta;
    pos_y -= obj->g_speed * delta;
  }
  if (highest_point > pos_y) {
    obj->g_speed = 0.0f;
    pos_y = highest_point;
  }
  obj->pos.y = pos_y;
    
  if (repeat)
    move_game_object(obj, remaining);
}

void draw_background() {
  float mul = 0.125 / cam_point.rot_v_pi;
  BeginShaderMode(basic2d.shader);
  DrawRectangleGradientV(0, 0, GAME_W, GAME_H * 4 / 7 * mul, color_d(0x99 / 3, 0x0, 0xff / 3, 0xff), BLACK);
  EndShaderMode();
}

void draw_chunks(WorldChunk *origin) {
  uqueue_push(&active_chunks, &origin);
  WorldChunk *chunk;
  bool full = false;
  while (!full && uqueue_pop(&active_chunks, &chunk)) {
    for (int i = 0; i < 4; i++) {
      WorldChunk *c = chunk->neighbours[i];
      if (c != NULL && !uqueue_push(&active_chunks, &c))
        full = true;
    }
  }
  uqueue_restore(&active_chunks);
  while (uqueue_pop(&active_chunks, &chunk))
    DrawModel(chunk->model, (Vector3){chunk->w_pos[0] * CHUNK_SIZE, 0.0f, chunk->w_pos[1] * CHUNK_SIZE}, 1.0f, chunk->tint);
  uqueue_reset(&active_chunks);
}

Rectangle get_game_object_frame(GameObject *obj) {
  Rectangle frame = {0};
  frame.width = obj->sprite_size[0];
  frame.height = obj->sprite_size[1];
  Animation *anim = obj->animations + obj->animation_index;
  frame.x = (int)anim->frame_index * obj->sprite_size[0];
  if (frame.x >= anim->texture.width) {
    frame.x = 0.0f;
    anim->frame_index -= anim->texture.width / obj->sprite_size[0];
  }
  if (obj->c_facings < 2)
    frame.y = 0.0f;
  else {
    int facing_index = -1;
    float max_dot = -1.0f;
    Vector2 rotated_dir = Vector2Rotate(vector3_xz(obj->last_move_dir), cam_point.rot_pi * PI);
    for (int i = 0; i < obj->c_facings; i++) {
      float dot = Vector2DotProduct(vector3_xz(obj->facings[i]), rotated_dir);
      dot = round(dot * 100.0f) / 100.0f;
      if (dot > max_dot) {
        max_dot = dot;
        facing_index = i;
      }
    }
    frame.y = facing_index * obj->sprite_size[1];
  }
  return frame;
}

void draw_game_object(GameObject *obj) {
  DrawBillboardPro(
    cam_point.cam,
    obj->animations[obj->animation_index].texture,
    get_game_object_frame(obj),
    Vector3Add(obj->pos, (Vector3){0.0f, obj->sprite_size[1] / 2 / TILE_SIZE / cam_point.cos_rot_v, 0.0f}),
    (Vector3){0.0f, 1.0f, 0.0f}, //cam_point.rel_up,
    (Vector2){MAX(obj->sprite_size[0], obj->sprite_size[1]) / TILE_SIZE, MAX(obj->sprite_size[0], obj->sprite_size[1]) / TILE_SIZE / cam_point.cos_rot_v},
    (Vector2){0.0f, 0.0f},
    0.0f, obj->tint
  );
}

void update_draw() {
  delta = GetFrameTime();
  next_turn = false;
  turn_keeper += delta * TPS;
  if (turn_keeper >= 1.0f) {
    turn_keeper -= 1.0f;
    next_turn = true;
  }
  screen_scale = MIN((float)GetScreenWidth() / GAME_W, (float)GetScreenHeight() / GAME_H);
  
  input.move_speed = INV_DIVINE * 10.0f;
  input.move_translate = Vector3Zero();
  input.cam_rotate = 0.0f;
  input.cam_rotate_v = 0.0f;
  input.zoom_factor = 0.0f;
  
#ifndef PLATFORM_ANDROID
  process_keyboard();
  process_mouse();
#endif
  process_controller();
#if defined(PLATFORM_WEB) || defined(PLATFORM_ANDROID)
  process_touch();
#endif
  
  //update
  move_game_object(&test_object, Vector2Scale(vector3_xz(input.move_translate), delta));
  
  if (test_object.animation_index == 0) {
    if (Vector3Equals(input.move_translate, Vector3Zero()))
      test_object.animations[test_object.animation_index].frame_index = 0.0f;
    else
      test_object.animations[test_object.animation_index].frame_index += 10.0f * delta;
  }
  cam_point_update(Vector3Scale(input.move_translate, delta), input.cam_rotate * delta, input.cam_rotate_v * delta, input.zoom_factor * delta);
  
  if (light_switch) {
    SetShaderValue(basic3d.shader, basic3d.light_src_loc, &cam_point.cam.target, SHADER_UNIFORM_VEC3);
  }
  else {
    float light_source[3] = {
      cam_point.cam.target.x + sin(GetTime() / 20) * 10000.0f,
      cam_point.cam.target.y + cos(GetTime() / 20) * 10000.0f,
      cam_point.cam.target.z + cos(GetTime() / 80) * 2000.0f
    };
    SetShaderValue(basic3d.shader, basic3d.light_src_loc, light_source, SHADER_UNIFORM_VEC3);
  }
  
  //draw
  BeginTextureMode(render_target);
    ClearBackground(BLACK);
    draw_background();
    BeginMode3D(cam_point.cam);
      SetShaderValue(basic3d.shader, basic3d.with_texture_loc, (int[1]){0}, SHADER_UNIFORM_INT);
      draw_chunks(test_object.current_chunk);
      BeginShaderMode(basic3d.shader);
        SetShaderValue(basic3d.shader, basic3d.with_texture_loc, (int[1]){1}, SHADER_UNIFORM_INT);
        draw_game_object(&test_object);
      EndShaderMode();
    EndMode3D();
  EndTextureMode();
  
  BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(
      render_target.texture,
      (Rectangle){0.0f, 0.0f, (float)render_target.texture.width, (float)-render_target.texture.height},
      (Rectangle){(GetScreenWidth() - ((float)GAME_W * screen_scale)) * 0.5f, (GetScreenHeight() - ((float)GAME_H * screen_scale)) * 0.5f,
      (float)GAME_W * screen_scale, (float)GAME_H * screen_scale}, (Vector2){0, 0}, 0.0f, WHITE
    );
    DrawFPS(10, 10);
    DrawText(TextFormat("xz(%.2f; %.2f)", cam_point.cam.target.x, cam_point.cam.target.z), 10, 40, 20, color_d(0xff, 0xff, 0x0, 0xff));
    DrawText(TextFormat("cam(%.2f; %.2f, %.2f)", cam_point.rot_pi, cam_point.rot_v_pi, cam_point.zoom), 10, 70, 20, color_d(0xff, 0xff, 0x0, 0xff));
    DrawText(light_switch ? "Torch" : "Sun", 10, 100, 20, color_d(0xff, 0xff, 0xff, 0xff));
    DrawText(TextFormat("%x", test_object.current_chunk), 10, 130, 20, color_d(0xcc, 0xff, 0xcc, 0xff));
    if (next_turn)
      DrawText("boop", 10, 160, 20, color_d(0xcc, 0xcc, 0xff, 0xff));
    DrawText(TextFormat("%f", get_chunk_height_at(test_object.current_chunk, vector3_xz(test_object.pos))), 10, 190, 20, color_d(0xcc, 0xcc, 0xff, 0xff));
  EndDrawing(); 
}

int main(void) {
  //init
#ifdef PLATFORM_WEB
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#else
  SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
#endif
  InitWindow(INITIAL_SCREEN_W, INITIAL_SCREEN_H, "Shokku-Yitsee");

#ifndef PLATFORM_WEB
  Image icon_image;
  icon_image = LoadImage("./res/textures/icon.png");
  SetWindowIcon(icon_image);
  UnloadImage(icon_image);
#endif
  
  SetWindowMinSize(GAME_W, GAME_H);
  render_target = LoadRenderTexture(GAME_W, GAME_H);
  SetTextureFilter(render_target.texture, TEXTURE_FILTER_POINT);
  
  SetGesturesEnabled(GESTURE_HOLD | GESTURE_DRAG);
  SetExitKey(KEY_NULL);
  SetTargetFPS(FPS);
  
  BeginDrawing();
    ClearBackground((Color){0x11, 0x00, 0x22, 0xff});
    DrawText("made with raylib", INITIAL_SCREEN_W / 2 - 100, INITIAL_SCREEN_H / 2 - 16, 32, RAYWHITE);
  EndDrawing();
  setup();
  
  //loop
#ifdef PLATFORM_WEB
  emscripten_set_main_loop(update_draw, 0, 1);
#else
  while (!WindowShouldClose())
    update_draw();
#endif
  
  //deinit
  UnloadShader(basic3d.shader);
  //UnloadModel(test_chunk.model);
  UnloadRenderTexture(render_target);
  CloseWindow();
  
  return 0;
}
