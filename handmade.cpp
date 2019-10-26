#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct BackBuffer {
  SDL_Texture *Texture;
  void *Pixels;
  int Width;
  int Height;
  int Pitch;
};

static BackBuffer Buffer;

void SDLResizeTexture(SDL_Renderer *Renderer, BackBuffer *Buffer, int Width, int Height) {
  if (Buffer->Texture) {
    SDL_DestroyTexture(Buffer->Texture);
  }

  if (Buffer->Pixels) {
    free(Buffer->Pixels);
  }

  Buffer->Texture = SDL_CreateTexture(Renderer,
                              SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING,
                              Width, Height);

  int BytesPerPixel = 4;

  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->Pitch = Width * BytesPerPixel;
  Buffer->Pixels = malloc(Height * Buffer->Pitch);
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
        } break;
      }
    } break;
  }
  return ShouldQuit;
}


void SDLUpdatePixels(BackBuffer Buffer) {
  uint8_t *Row = (uint8_t *)Buffer.Pixels;
  for (int Y = 0; Y < (Buffer.Height); ++Y) {

    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < (Buffer.Width); ++X) {
      uint8_t Blue = (X);
      uint8_t Green = (Y);
      uint8_t Red = 0;

      if (X % 128 < 3 || Y % 256 < 4) {
        Red = 255;
      }

      *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
    }

    Row += Buffer.Pitch;
  }
}

void SDLUpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer, BackBuffer buffer) {
  SDL_UpdateTexture(Buffer.Texture, NULL, Buffer.Pixels, Buffer.Pitch);
  SDL_RenderCopy(Renderer, Buffer.Texture, 0, 0);

  SDL_RenderPresent(Renderer);
}

int main() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("%s\n", SDL_GetError());
    exit(1);
  }

  int Width = 1280, Height = 720;

  SDL_Window *Window = SDL_CreateWindow("Handmade",
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        640, 480,
                                        SDL_WINDOW_RESIZABLE);
  SDL_Renderer *Renderer = SDL_CreateRenderer(Window, -1, 0);
  SDLResizeTexture(Renderer, &Buffer, Width, Height);

  bool Running = true;

  while(Running) {
    SDL_Event Event;

    while(SDL_PollEvent(&Event)) {
      if (HandleEvent(&Event)) {
        Running = false;
      }
    }

    SDLUpdatePixels(Buffer);
    SDLUpdateWindow(Window, Renderer, Buffer);
  }

  if (Buffer.Texture) { SDL_DestroyTexture(Buffer.Texture); }
  if (Buffer.Pixels) { free(Buffer.Pixels); }
  SDL_Quit();
}

