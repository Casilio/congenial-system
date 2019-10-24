#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

SDL_Texture *Texture;
void *Pixels;
int TextureWidth;
int BytesPerPixel = 4;

bool HandleEvent(SDL_Event *Event);
void SDLResizeTexture(SDL_Renderer *Renderer, int Width, int Heigth);

int main() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("%s\n", SDL_GetError());
    exit(1);
  }

  int Width = 640, Height = 480;

  SDL_Window *Window = SDL_CreateWindow("Handmade",
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        640, 480,
                                        SDL_WINDOW_RESIZABLE);
  SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, 0);
  SDLResizeTexture(Renderer, Width, Height);

  for(;;) {
    SDL_Event Event;
    SDL_WaitEvent(&Event);

    if (HandleEvent(&Event)) {
      break;
    }
  }

  if (Texture) { SDL_DestroyTexture(Texture); }
  if (Pixels) { free(Pixels); }
  SDL_Quit();
}

bool HandleEvent(SDL_Event *Event) {
  bool ShouldQuit = false;

  switch(Event->type) {
    case SDL_QUIT: {
      ShouldQuit = true;
    } break;
    case SDL_WINDOWEVENT: {
      switch(Event->window.event) {
        case SDL_WINDOWEVENT_RESIZED: {
          SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
          SDL_Renderer *Renderer = SDL_GetRenderer(Window);
          int Width, Height;
          SDL_GetWindowSize(Window, &Width, &Height);

          SDLResizeTexture(Renderer, Width, Height);
        } break;

        case SDL_WINDOWEVENT_EXPOSED: {
          SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
          SDL_Renderer *Renderer = SDL_GetRenderer(Window);
          int Width, Height;
          SDL_GetWindowSize(Window, &Width, &Height);

          int Pitch = Width * BytesPerPixel;
          uint8_t *Row = (uint8_t *)Pixels;
          for (int Y = 0; Y < Height; ++Y) {

            uint32_t *Pixel = (uint32_t *)Row;
            for (int X = 0; X < Width; ++X) {
              uint8_t Blue = (X);
              uint8_t Green = (Y);

              *Pixel++ = ((Green << 8) | Blue);
            }

            Row += Pitch;

          }

          SDL_UpdateTexture(Texture, 0, Pixels, TextureWidth * BytesPerPixel);
          SDL_RenderCopy(Renderer, Texture, 0, 0);

          SDL_RenderPresent(Renderer);
        } break;
      }
    } break;
  }
  return ShouldQuit;
}

void SDLResizeTexture(SDL_Renderer *Renderer, int Width, int Height) {
  if (Texture) {
    SDL_DestroyTexture(Texture);
  }
  if (Pixels) {
    free(Pixels);
  }

  Texture = SDL_CreateTexture(Renderer,
                              SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING,
                              Width, Height);
  Pixels = malloc(Width * Height * BytesPerPixel);
}
