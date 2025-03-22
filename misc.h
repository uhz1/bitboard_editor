#pragma once

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#define HEX 0x0
#define INT 0x1
#define BIN 0x2

typedef enum
{
  _8BIT,
  _16BIT,
  _32BIT,
  _64BIT,
  _128BIT,
  GRID_SIZE,
  COPY,
  ROT_LEFT,
  ROT_RIGHT,
  MOVE_NORTH,
  MOVE_NORTHEAST,
  MOVE_EAST,
  MOVE_SOUTHEAST,
  MOVE_SOUTH,
  MOVE_SOUTHWEST,
  MOVE_WEST,
  MOVE_NORTHWEST,
  FLIP,
  MIRROR,
  CLEAR,
  NUM_DISPLAY,
  BITGRID_BLOCK,
} BlockType;

typedef enum
{
  BLOCK,
  DROPDOWN,
  GRID,
} ItemType;

typedef struct
{
  int extra_info;
  int is_hovered;
  BlockType type;
  SDL_Rect r;
  SDL_Color color;
  SDL_Texture *texture;
} Block;

typedef struct
{
  int is_open;
  int selected;
  int item_cnt;
  Block menu;
  Block *items;
} DropdownMenu;

typedef struct
{
  unsigned long long high;
  unsigned long long low;
} uint128_t;

typedef struct
{
  int rows;
  int cols;
  uint128_t state;
  SDL_Rect dim; // x,y -> start grid | w,h individual block w/h
  Block *grid;
} BitGrid;

typedef struct
{
  ItemType type;
  void *item;
} Item;

typedef struct
{
  int cur_sz;
  int max_sz;
  Item *items;
} ItemManager;

SDL_Texture *create_text_texture(SDL_Renderer *renderer, TTF_Font *font, 
                                        const char *txt);

void clean(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font, int depth);

void error_and_quit(const char* msg, const char* sdl_err, SDL_Window* window,
                    SDL_Renderer* renderer, TTF_Font* font, int depth);

void init(SDL_Window** window, SDL_Renderer** renderer, TTF_Font** font, 
          int window_width, int window_height);

Block *add_block(SDL_Renderer *r, TTF_Font *f, ItemManager *im, BlockType type, 
               int x, int y, int w, int h, SDL_Color c, const char* text);

void add_dropdown(SDL_Renderer *r, TTF_Font *f, ItemManager *im, BlockType type, 
               int x, int y, int w, int h, SDL_Color c, const char* text, int item_cnt);

void add_grid(ItemManager *im, int rows, int cols, int x, int y, int w, int h);

char *grid_state(const BitGrid *bg, int type);

ItemManager *create_item_manager(int max_sz);

void *find_item(ItemManager *im, ItemType it, BlockType bt);

void delete_item_manager(ItemManager *im);
