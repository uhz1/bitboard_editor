#include "SDL2/SDL_render.h"
#include "assert.h"
#include "misc.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include <complex.h>
#include <stdio.h>

void clean(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font, int depth)
{
  switch (depth)
  {
    case 4:
      TTF_CloseFont(font);
    case 3:
      SDL_DestroyRenderer(renderer);
    case 2:
      SDL_DestroyWindow(window); 
    case 1:
      TTF_Quit();
    case 0:
      SDL_Quit();
    default:
      exit(1);
  }
}

void error_and_quit(const char* msg, const char* sdl_err, SDL_Window* window,
                    SDL_Renderer* renderer, TTF_Font* font, int depth)
{
  fprintf(stderr, "%s: %s\n", msg, sdl_err);
  clean(window, renderer, font, depth);
}

void init(SDL_Window** window, SDL_Renderer** renderer, TTF_Font** font, 
          int window_width, int window_height)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    error_and_quit("SDL Video Init error", SDL_GetError(), 
                NULL, NULL, NULL, -1);

  if (TTF_Init() != 0)
    error_and_quit("SDL TTF Init error", TTF_GetError(), 
                NULL, NULL, NULL, 0);

  *window = SDL_CreateWindow("Herm's Bitboard Editor",
                                        SDL_WINDOWPOS_CENTERED, 
                                        SDL_WINDOWPOS_CENTERED,
                                        window_width,
                                        window_height,
                                        SDL_WINDOW_SHOWN);

  if (*window == NULL)
    error_and_quit("SDL_CreateWindow error", SDL_GetError(),
                   NULL, NULL, NULL, 1);

  *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);

  if (*renderer == NULL)
    error_and_quit("SDL_CreateRenderer error", SDL_GetError(),
                   *window, NULL, NULL, 2);

  *font = TTF_OpenFont("ComicShannsMonoNerdFont-Regular.otf", 22);

  if (*font == NULL)
    error_and_quit("TTF_OpenFont error", TTF_GetError(),
                   *window, *renderer, NULL, 3);
}

SDL_Texture *create_text_texture(SDL_Renderer *renderer, TTF_Font *font, 
                                        const char *txt)
{
  if (txt)
  {
    SDL_Surface* s = TTF_RenderText_Blended(font, txt, (SDL_Color){0xff, 0xff, 0xff, 0xff});
    assert(s);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, s);
    SDL_FreeSurface(s);
    assert(texture);
    return texture;
  }
  return NULL;
}

static void add_item(ItemManager *im, void *item, ItemType type)
{
  if (im->cur_sz >= im->max_sz)
  {
    Item *new_items = realloc(im->items, 2 * im->max_sz * sizeof(Item));
    /*
    Item *new_items = malloc(2 * im->max_sz * sizeof(Item));
    for (int i = 0; i < im->cur_sz; ++i)
      new_items[i] = im->items[i];
    free(im->items); */
    assert(new_items);
    im->items = new_items;
    im->max_sz *= 2;
  }
  im->items[im->cur_sz].type = type;
  im->items[im->cur_sz++].item = item;
}

Block *add_block(SDL_Renderer *r, TTF_Font *f, ItemManager *im, BlockType type, 
               int x, int y, int w, int h, SDL_Color c, const char* text)
{
  Block *b = malloc(sizeof(Block));
  b->color = c;
  b->type = type;
  b->is_hovered = 0;
  b->r = (SDL_Rect){x, y, w, h};
  b->texture = create_text_texture(r, f, text);
  add_item(im, b, BLOCK);
  return b;
}

static void clean_block(Block *b, int manual_alloc)
{
  if (b->texture)
    SDL_DestroyTexture(b->texture);
  if (manual_alloc)
    free(b);
}

void add_dropdown(SDL_Renderer *r, TTF_Font *f, ItemManager *im, BlockType type, 
               int x, int y, int w, int h, SDL_Color c, const char* text, int item_cnt)
{
  DropdownMenu *dm = malloc(sizeof(DropdownMenu));
  dm->is_open = 0;
  Block *b = &dm->menu;
  b->color = c;
  b->type = type;
  b->is_hovered = 0;
  b->r = (SDL_Rect){x, y, w, h};
  b->texture = create_text_texture(r, f, text);
  dm->item_cnt = item_cnt;
  dm->items = malloc(item_cnt * sizeof(Block));
  add_item(im, dm, DROPDOWN);
}

static void clean_dropdown(DropdownMenu *dm)
{
  for (int i = 0; i < dm->item_cnt; ++i)
    clean_block(&dm->items[i], 0);
  dm->menu.texture = NULL;
  clean_block(&dm->menu, 0);
  free(dm->items);
  free(dm);
}

void add_grid(ItemManager *im, int rows, int cols, int x, int y, int w, int h)
{
  BitGrid *bg = malloc(sizeof(BitGrid));
  bg->state.low = bg->state.high = 0;
  bg->rows = rows;
  bg->cols = cols;
  bg->dim = (SDL_Rect){x, y, w, h};
  bg->grid = malloc(rows * cols * sizeof(Block));
  for (int i = 0; i < rows * cols; ++i)
  {
    bg->grid[i].is_hovered = 0;
    bg->grid[i].texture = NULL;
    bg->grid[i].type = BITGRID_BLOCK;
    bg->grid[i].color = (SDL_Color){0xff, 0xff, 0xff, 0xff};
  }
  add_item(im, bg, GRID);
}

static void clean_grid(BitGrid *bg)
{
  for (int i = 0; i < bg->rows * bg->cols; ++i)
    clean_block(&bg->grid[i], 0);
  free(bg->grid);
  free(bg);
}

char *grid_state(const BitGrid *bg, int type)
{
  char *p;
  static char buf[131];
  memset(buf, '0', sizeof(buf));
  if (type == HEX)
  {
    p = buf + 2;
    sprintf(buf, "0x%016llx%016llx", bg->state.high, bg->state.low);
  }
  else if (type == INT)
  {
    p = buf;
    int carry;
    buf[40] = '\0';
    uint128_t n = bg->state;
    for (int i = 0; i < 128; ++i)
    {
      carry = (n.high & (1ULL << 63)) != 0;
      n.high = (n.high << 1) | (n.low >> 63);
      n.low <<= 1;

      for (int j = 39; j >= 0; --j)
      {
        buf[j] += buf[j] - '0' + carry;
        carry = buf[j] > '9';

        if (carry)
          buf[j] -= 10;
      }
    }
  }
  else if (type == BIN)
  {
    p = buf + 2;
    buf[0] = '0';
    buf[1] = 'b';
    int pos = 2;
    for (int i = 63; i >= 0; --i)
      buf[pos++] = (bg->state.high & (1ULL << i)) ? '1' : '0';
    for (int i = 63; i >= 0; --i)
      buf[pos++] = (bg->state.low & (1ULL << i)) ? '1' : '0';
    buf[pos] = '\0';
  }
  while (*p == '0')
    p++;

  if (*p == '\0')
    if (type == INT)
      --p;

  if (type != INT)
  {
    *(--p) = type == HEX ? 'x' : 'b';
    *(--p) = '0';
  }

  if (p != buf)
    memmove(buf, p, strlen(p) + 1);
  //printf("Buf ;%s;\n", buf);
  return buf;
}

ItemManager *create_item_manager(int max_sz)
{
  ItemManager *im = malloc(sizeof(ItemManager));
  im->cur_sz = 0;
  im->max_sz = max_sz;
  im->items = malloc(max_sz * sizeof(Item));
  return im;
}

void *find_item(ItemManager *im, ItemType it, BlockType bt)
{
  for (int i = 0; i < im->cur_sz; ++i)
  {
    switch (it)
    {
    case BLOCK:
      if (((Block*)im->items[i].item)->type == bt)
        return im->items[i].item;
      break;
    case DROPDOWN:
      return im->items[i].item;
    case GRID:
      return im->items[i].item;
    }
  }
  return NULL;
}

void delete_item_manager(ItemManager *im)
{
  for (int i = 0; i < im->cur_sz; ++i)
  {
    switch (im->items[i].type)
    {
    case BLOCK:
      clean_block(im->items[i].item, 1);
      break;
    case DROPDOWN:
      clean_dropdown(im->items[i].item);
      break;
    case GRID:
      clean_grid(im->items[i].item);
      break;
    }
  }
  free(im->items);
  free(im);
}
