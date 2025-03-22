
#include "SDL2/SDL_events.h"
#include "SDL2/SDL_mouse.h"
#include "SDL2/SDL_render.h"
#include "assert.h"
#include "stdint.h"
#include "stdio.h"

#include "SDL2/SDL_ttf.h"

#include "misc.h"

#define ALL 4
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define BLOCK_SZ 100

#define MIN(x, y) ((x) > (y)) ? (y) : (x)

int coords_in_rect(SDL_Rect *r, int x, int y)
{
  return r->x <= x && x <= r->x + r->w && r->y <= y && y <= r->y + r->h;
}

int no_rect_overlap(ItemManager *im, SDL_Rect *r1)
{
  for (int i = 0; i < im->cur_sz; ++i)
  {
    if (im->items[i].type == DROPDOWN)
    {
      DropdownMenu *dm = im->items[i].item;
      if (dm->is_open)
      {
        for (int j = 0; j < dm->item_cnt; ++j)
        {
          SDL_Rect *r2 = &dm->items[j].r;
          if (r1->x < r2->x + r2->w && r2->x < r1->x + r1->w &&
              r1->y < r2->y + r2->h && r2->y < r1->y + r1->h)
            return 0;
        }
      }
    }
  }
  return 1;
}

void render_block(SDL_Renderer *r, TTF_Font *f, ItemManager *im, Block *b)
{
  if (b->type == NUM_DISPLAY)
  {
    BitGrid *bg = find_item(im, GRID, 0);
    assert(bg);
    const char *c = grid_state(bg, b->extra_info);
    SDL_DestroyTexture(b->texture);
    b->texture = create_text_texture(r, f, c);
    SDL_QueryTexture(b->texture, NULL, NULL, &b->r.w, &b->r.h);
  }

  SDL_SetRenderDrawColor(r, b->color.r, b->color.g, b->color.b, b->color.a);
  SDL_RenderDrawRect(r, &b->r);

  if (b->texture)
  {
    SDL_Rect tdim;
    SDL_QueryTexture(b->texture, NULL, NULL, &tdim.w, &tdim.h);
    tdim.x = b->r.x + (b->r.w - tdim.w) / 2;
    tdim.y = b->r.y + (b->r.h - tdim.h) / 2;

    if (b->is_hovered)
      SDL_SetTextureAlphaMod(b->texture, 180);
    else
      SDL_SetTextureAlphaMod(b->texture, 255);

    SDL_RenderCopy(r, b->texture, NULL, &tdim);
  }
}

void render_dropdown(SDL_Renderer *r, ItemManager *im, DropdownMenu *dm, int *row, int *col)
{
  *row = dm->selected == _128BIT ? 8 : pow(2, dm->selected);
  *col = dm->selected == _128BIT ? 16 : 8;

  dm->menu.texture = dm->items[dm->selected].texture;
  render_block(r, NULL, im, &dm->menu);
  if (dm->is_open)
  {
    for (int i = 0; i < dm->item_cnt; ++i)
      render_block(r, NULL, im, &dm->items[i]);
  }
}

void render_grid(SDL_Renderer *r, TTF_Font *f, ItemManager *im, BitGrid *bg, int rows, int cols)
{
  rows = rows >= 1 ? rows : 1;
  cols = cols >= 1 ? cols : 1;

  if (rows != bg->rows || cols != bg->cols)
  {
    free(bg->grid);
    bg->grid = malloc(rows * cols * sizeof(Block));
    bg->state.high = bg->state.low = 0;
    for (int i = 0; i < rows * cols; ++i)
    {
      bg->grid[i].is_hovered = 0;
      bg->grid[i].texture = NULL;
      bg->grid[i].color = (SDL_Color){0xff, 0xff, 0xff, 0xff};
    }
    bg->rows = rows;
    bg->cols = cols;
  }

  int padding = 1;

  bg->dim.w = (WINDOW_WIDTH - bg->dim.x) / bg->cols - padding;
  bg->dim.h = (WINDOW_HEIGHT - bg->dim.y) / bg->rows - padding;

  bg->dim.w = MIN(bg->dim.w, bg->dim.h);

  if (bg->dim.w <= 0)
    assert(0);

  bg->dim.h = bg->dim.w;

  int gx = bg->dim.x + ((WINDOW_WIDTH - bg->dim.x) - (bg->dim.w + padding) * bg->cols) / 2,
      gy = bg->dim.y + ((WINDOW_HEIGHT - bg->dim.y) - (bg->dim.h + padding) * bg->rows) / 2;

  for (int i = 0; i < bg->rows * bg->cols; ++i)
  {
    int row = i / bg->cols, col = i % bg->cols;
    SDL_Rect *gdim = &bg->grid[i].r;
    gdim->x = gx + (bg->dim.w + padding) * col;
    gdim->y = gy + (bg->dim.h + padding) * row;
    gdim->w = bg->dim.w;
    gdim->h = bg->dim.h;

    SDL_Color *color = &bg->grid[i].color;
    SDL_SetRenderDrawColor(r, color->r, color->g, color->b, color->a);
    if (bg->grid[i].is_hovered)
      SDL_RenderFillRect(r, gdim);
    else
      SDL_RenderDrawRect(r, gdim);
  }
}

SDL_Color rc() { return (SDL_Color){rand() % 256, rand() % 256, rand() % 256, 0xff}; }

void init_ui_layout(SDL_Renderer *r, TTF_Font *f, ItemManager *im)
{
  SDL_Color white = {0xff, 0xff, 0xff, 0xff};
  SDL_Color black = {0, 0, 0, 0xff};
  int padding = 5;
  int x = padding, y = WINDOW_HEIGHT / 4, w = WINDOW_WIDTH / 9, h = WINDOW_HEIGHT / 13;

  add_grid(im, 8, 8, WINDOW_WIDTH / 8, WINDOW_HEIGHT / 11, BLOCK_SZ, BLOCK_SZ);

  // add number into string here
  char *t = grid_state(find_item(im, GRID, 0), 0);
  Block *b = add_block(r, f, im, NUM_DISPLAY, x + 300, x, w, h, rc(), t);
  b->extra_info = HEX;
  SDL_QueryTexture(b->texture, NULL, NULL, &b->r.w, &b->r.h);

  const char *texts[] = {"8-Bit", "16-Bit", "32-Bit", "64-Bit", "128-Bit"};
  const int types[] = {_8BIT, _16BIT, _32BIT, _64BIT, _128BIT};
  add_dropdown(r, f, im, GRID_SIZE, x, y, w, h, rc(), texts[3], 5);
  DropdownMenu *dm = im->items[im->cur_sz - 1].item;
  dm->selected = 3;
  for (int i = 0; i < 5; ++i)
  {
    dm->items[i].r = (SDL_Rect){x, y + h + (h/2 + 1) * i, w, h / 2};
    dm->items[i].is_hovered = 0;
    dm->items[i].texture = create_text_texture(r, f, texts[i]);
    dm->items[i].color = rc();
    dm->items[i].type = types[i];
  }
  y += h + padding;

  add_block(r, f, im, COPY, x, y, w, h, rc(), "Copy");
  y += h + padding;

  int half_w = (w - padding) / 2;
  add_block(r, f, im, ROT_LEFT, x, y, half_w, h, rc(), "RL");
  add_block(r, f, im, ROT_LEFT, x + half_w + padding, y, half_w, h, rc(), "RR");
  y += h + padding;

  half_w = (w - padding * 2) / 3;
  int cnt = 0;
  const char *ttypes[] = {"NW", "N", "NE", "W", "E", "SW", "S", "SE"};
  const int itypes[] = {MOVE_NORTHWEST, MOVE_NORTH, MOVE_NORTHEAST, 
                        MOVE_WEST,                  MOVE_EAST,
                        MOVE_SOUTHWEST, MOVE_SOUTH, MOVE_SOUTHEAST};
  for (int i = 0; i < 3; ++i, ++cnt)
    add_block(r, f, im, itypes[cnt], x + i * (half_w + padding), y, half_w, h, rc(), ttypes[cnt]);
  y += h + padding;

  add_block(r, f, im, itypes[cnt], x, y, half_w, h, rc(), ttypes[cnt]);
  cnt++;
  add_block(r, f, im, itypes[cnt], x + 2 * (half_w + padding), y, half_w, h, rc(), ttypes[cnt]);
  cnt++;
  y += h + padding;

  for (int i = 0; i < 3; ++i, ++cnt)
    add_block(r, f, im, itypes[cnt], x + i * (half_w + padding), y, half_w, h, rc(), ttypes[cnt]);
  y += h + padding;

  add_block(r, f, im, FLIP, x, y, w, h, rc(), "Flip");
  y += h + padding;

  add_block(r, f, im, CLEAR, x, y, w, h, rc(), "Clear");
}

// fix, bad
void handle_grid_fill(BitGrid *bg, int j)
{
  if (bg->cols == 16)
  {
    if (j <= 63)
      bg->state.high ^= 1ULL << (63 - j);
    else
      bg->state.low ^= 1ULL << (127 - j);
  }
  else
    bg->state.low ^= 1ULL << (bg->rows * bg->cols - 1 - j);
}

void handle_mousemotion(ItemManager *im, int mx, int my, int pmx, int pmy, int mouse_down)
{
  Block *b;
  DropdownMenu *dm;
  BitGrid *bg;
  for (int i = 0; i < im->cur_sz; ++i)
  {
    switch (im->items[i].type)
    {
    case BLOCK:
      b = im->items[i].item;
      if (no_rect_overlap(im, &b->r) && coords_in_rect(&b->r, mx, my))
        b->is_hovered = 1;
      else
        b->is_hovered = 0;
      break;
    case DROPDOWN:
      dm = im->items[i].item;
      if (coords_in_rect(&dm->menu.r, mx, my))
        dm->menu.is_hovered = 1;
      else
        dm->menu.is_hovered = 0;

      if (dm->is_open)
      {
        for (int i = 0; i < dm->item_cnt; ++i)
          if (coords_in_rect(&dm->items[i].r, mx, my))
            dm->items[i].is_hovered = 1;
          else
            dm->items[i].is_hovered = 0;
      }
      break;
    case GRID:
      bg = (BitGrid*)im->items[i].item;
      for (int j = 0; j < bg->rows * bg->cols; ++j)
      {
        if (coords_in_rect(&bg->grid[j].r, mx, my) && mouse_down && 
            !coords_in_rect(&bg->grid[j].r, pmx, pmy))
        {
          bg->grid[j].is_hovered ^= 1;

          handle_grid_fill(bg, j);
        }
      }
      break;
    }
  }
}

void handle_block_click(ItemManager *im, Block *b, int x, int y)
{
  Block *block;
  BitGrid *bg = find_item(im, GRID, 0);
  if (coords_in_rect(&b->r, x, y))
  {
    switch (b->type) 
    {
    case COPY:
      block = find_item(im, BLOCK, NUM_DISPLAY);
      SDL_SetClipboardText(grid_state(bg, block ? block->extra_info : HEX));
      break;
    case ROT_LEFT:
      break;
    case ROT_RIGHT:
      break;
    case MOVE_NORTH:
      break;
    case MOVE_NORTHEAST:
      break;
    case MOVE_EAST:
      break;
    case MOVE_SOUTHEAST:
      break;
    case MOVE_SOUTH:
      break;
    case MOVE_SOUTHWEST:
      break;
    case MOVE_WEST:
      break;
    case MOVE_NORTHWEST:
      break;
    case FLIP:
      break;
    case MIRROR:
      break;
    case CLEAR:
      bg->state.high = bg->state.low = 0;
      for (int i = 0; i < bg->rows * bg->cols; ++i)
        bg->grid[i].is_hovered = 0;
      break;
    case NUM_DISPLAY:
      b->extra_info = (b->extra_info + 1) % 3;
      break;
    }
  }
}

void handle_dropdown_click(DropdownMenu *dm, int x, int y)
{
  if (coords_in_rect(&dm->menu.r, x, y))
  {
    dm->is_open ^= 1;
    return;
  }
  else if (dm->is_open)
  {
    dm->is_open = 0;
    for (int i = 0; i < dm->item_cnt; ++i)
    {
      if (coords_in_rect(&dm->items[i].r, x, y))
      {
        dm->selected = i;
        return;
      }
    }
  }
  else
    dm->is_open = 0;
}

void handle_grid_click(BitGrid *bg, int x, int y)
{
  for (int j = 0; j < bg->rows * bg->cols; ++j)
  {
    if (coords_in_rect(&bg->grid[j].r, x, y))
    {
      bg->grid[j].is_hovered ^= 1;

      handle_grid_fill(bg, j);
    }
  }
}

void handle_mouseclick(ItemManager *im, int x, int y)
{
  for (int i = 0; i < im->cur_sz; ++i)
  {
    switch (im->items[i].type)
    {
    case BLOCK:
      //if (no_rect_overlap(im, &((Block*)im->items[i].item)->r))
        handle_block_click(im, im->items[i].item, x, y);
      break;
    case DROPDOWN:
      handle_dropdown_click(im->items[i].item, x, y);
      break;
    case GRID:
      handle_grid_click(im->items[i].item, x, y);
      break;
    }
  }
}

int main()
{
  SDL_Window* window;
  SDL_Renderer* renderer;
  TTF_Font* font;

  init(&window, &renderer, &font, WINDOW_WIDTH, WINDOW_HEIGHT);

  SDL_Color black = {0, 0, 0, 0xff};
  SDL_Color white = {0xff, 0xff, 0xff, 0xff};
  SDL_Event event;

  int quit = 0, rows = 8, cols = 8, mouse_down = 0, prev_mx = 0, prev_my = 0;
  ItemManager *im = create_item_manager(1);
  init_ui_layout(renderer, font, im);

  while (!quit)
  {
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
        quit = 1;

      if (event.type == SDL_MOUSEBUTTONDOWN)
      {
        if (event.button.button == SDL_BUTTON_LEFT)
        {
          mouse_down = 1;
          handle_mouseclick(im, event.button.x, event.button.y);
        }
      }

      if (event.type == SDL_MOUSEMOTION)
      {
        handle_mousemotion(im, event.motion.x, event.motion.y, prev_mx, prev_my, mouse_down);
        prev_mx = event.motion.x;
        prev_my = event.motion.y;
      }

      if (event.type == SDL_MOUSEBUTTONUP)
      {
        if (event.button.button == SDL_BUTTON_LEFT)
          mouse_down = 0;
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int i = 0; i < im->cur_sz; ++i)
    {
      switch (im->items[i].type)
      {
      case BLOCK:
        if (no_rect_overlap(im, &((Block*)im->items[i].item)->r))
          render_block(renderer, font, im, im->items[i].item);
        break;
      case DROPDOWN:
        render_dropdown(renderer, im, im->items[i].item, &rows, &cols);
        break;
      case GRID:
        render_grid(renderer, font, im, im->items[i].item, rows, cols);
        break;
      }
    }

    SDL_RenderPresent(renderer);
  }

  delete_item_manager(im);
  clean(window, renderer, font, ALL);
}
