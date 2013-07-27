/******************************************************************************

  Curse of War -- Real Time Strategy Game for Linux.
  Copyright (C) 2013 Alexey Nikolaev.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
******************************************************************************/

#include "output-common.h"
#include "output-sdl.h"

Uint32 getpixel(SDL_Surface *surface, int x, int y) {
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

  switch(bpp) {
    case 1:
      return *p;
      break;

    case 2:
      return *(Uint16 *)p;
      break;

    case 3:
      if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;
      break;

    case 4:
      return *(Uint32 *)p;
      break;

    default:
      return 0;       /* shouldn't happen, but avoids warnings */
  }
}

void blit_subpic(SDL_Surface *src_surf, SDL_Surface *dst_surf, int srci, int srcj, int dsti, int dstj) {
  static SDL_Rect src, dest;
   
  src.x = srci * TILE_WIDTH;
  src.y = srcj * TILE_HEIGHT;
  src.w = TILE_WIDTH;
  src.h = TILE_HEIGHT;
   
  dest.x = dsti * TILE_WIDTH + dstj*(TILE_WIDTH/2);
  dest.y = dstj * TILE_HEIGHT;
  dest.w = TILE_WIDTH;
  dest.h = TILE_HEIGHT;
   
  SDL_BlitSurface(src_surf, &src, dst_surf, &dest);
}

void blit_subpic_noise(SDL_Surface *src_surf, SDL_Surface *dst_surf, int srci, int srcj, int dsti, int dstj, int variant) {
  static SDL_Rect src, dest;
   
  src.x = srci * TILE_WIDTH;
  src.y = srcj * TILE_HEIGHT;
  src.w = TILE_WIDTH;
  src.h = TILE_HEIGHT;
   
  int rangex = 1;
  int rangey = 1;
  int rndx = variant%(2*rangex+1) - rangex;
  int rndy = variant%(rangey+1);

  dest.x = dsti * TILE_WIDTH + dstj*(TILE_WIDTH/2) + rndx;
  dest.y = dstj * TILE_HEIGHT + rndy;
  dest.w = TILE_WIDTH;
  dest.h = TILE_HEIGHT;
   
  SDL_BlitSurface(src_surf, &src, dst_surf, &dest);
}

/* double height tile */
void blit_subpic_2h(SDL_Surface *src_surf, SDL_Surface *dst_surf, int srci, int srcj, int dsti, int dstj) {
  static SDL_Rect src, dest;
   
  src.x = srci * TILE_WIDTH;
  src.y = srcj * TILE_HEIGHT - TILE_HEIGHT;
  src.w = TILE_WIDTH;
  src.h = TILE_HEIGHT*2;
   
  dest.x = dsti * TILE_WIDTH + dstj*(TILE_WIDTH/2);
  dest.y = dstj * TILE_HEIGHT - TILE_HEIGHT;
  dest.w = TILE_WIDTH;
  dest.h = TILE_HEIGHT*2;
   
  SDL_BlitSurface(src_surf, &src, dst_surf, &dest);
}

#define POSY(j) ((j)+1)
#define POSX(ui,i) ((i) - (ui->xskip))

void output_sdl (SDL_Surface *tileset, SDL_Surface *screen, struct state *s, struct ui *ui,
    int variant[MAX_WIDTH][MAX_HEIGHT], int pop_variant[MAX_WIDTH][MAX_HEIGHT], int ktime) {

  /* Clear screen */
  SDL_Rect rect = {0, 0, screen->w, screen->h};
  SDL_FillRect(screen, &rect, (SDL_MapRGB(screen->format, 0, 0, 0)));

  /* Draw */
  int i=0,j=0;
  int srci=0, srcj=0;
  for (j=0; j<s->grid.height; ++j) {
    for (i=0; i<s->grid.width; ++i) {
      
      int owner = s->grid.tiles[i][j].pl;
      int done = 0; 
      switch (s->grid.tiles[i][j].cl) {
        case abyss: break; 
        default:
          /* draw grass */
          blit_subpic (tileset, screen, 0 + variant[i][j]%6, 0 + variant[i][j]/6%3, POSX(ui,i), POSY(j));
          /* draw everything else */
          switch (s->grid.tiles[i][j].cl) {
            case village: 
              srci=0; srcj=7 + 3*owner; 
              break;
            case town: 
              srci=1; srcj=7 + 3*owner; 
              break;
            case castle: 
              srci=2; srcj=7 + 3*owner; 
              break;
            case mountain: case mine:
              srci=0 + variant[i][j]%5; 
              srcj=5; break;
            case grassland: 
              {
                /* drawing population */
                int pop = s->grid.tiles[i][j].units[owner][citizen];
                if (pop > 0) {
                  srci = 0 + pop_to_symbol(pop);
                  srcj = 8 + 3*(owner);
                  blit_subpic_noise (tileset, screen, srci, srcj, POSX(ui, i), POSY(j), pop_variant[i][j]);
                  if (rand()%20 == 0) {
                    int d = 1;
                    if (owner != s->controlled) d = 11;
                    pop_variant[i][j] = (pop_variant[i][j] + d) % 10000;
                  }
                }
              }
              done = 1;
            default:
              done = 1;
          }
          if (!done) {
            blit_subpic_2h (tileset, screen, srci, srcj, POSX(ui, i), POSY(j));
            if (s->grid.tiles[i][j].cl == mine) {
              if (s->grid.tiles[i][j].pl > 0)
                /* add a currency sign if its mining */
                blit_subpic_2h (tileset, screen, 5, 5, POSX(ui, i), POSY(j));
              else
                /* no currency sign */
                blit_subpic (tileset, screen, 5, 5, POSX(ui, i), POSY(j));
            }
          }
      }
      /* flags */
      int p;
      for (p=0; p<MAX_PLAYER; ++p) {
        if (p != s->controlled) {
          if (s->fg[p].flag[i][j] != 0 && ((ktime/5 + p) / 5)%10 < 10) {
            blit_subpic_2h (tileset, screen, 4, 7+3*p, POSX(ui, i), POSY(j));
          }
        }
        else 
          if (s->fg[p].flag[i][j] != 0) {
            blit_subpic_2h (tileset, screen, 3, 7+3*p, POSX(ui, i), POSY(j));
          }
      }

    }
  }


  /* draw cursor */
  blit_subpic_2h (tileset, screen, 6, 5, POSX(ui, ui->cursor.i-1), POSY(ui->cursor.j));
  blit_subpic_2h (tileset, screen, 7, 5, POSX(ui, ui->cursor.i), POSY(ui->cursor.j));
  blit_subpic_2h (tileset, screen, 8, 5, POSX(ui, ui->cursor.i+1), POSY(ui->cursor.j));

}