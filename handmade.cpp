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
  }
  return ShouldQuit;
}


void SDLUpdatePixels(BackBuffer Buffer, int XOffset, int YOffset) {
  uint8_t *Row = (uint8_t *)Buffer.Pixels;
  for (int Y = 0; Y < (Buffer.Height); ++Y) {

    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < (Buffer.Width); ++X) {
      uint8_t Blue = (X + XOffset);
      uint8_t Green = (Y + YOffset);
      uint8_t Red = 128;

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

  int XOffset = 0, YOffset = 0;
  int speed = 3;

  while(Running) {
    SDL_Event Event;

    while(SDL_PollEvent(&Event)) {
      if (HandleEvent(&Event)) {
        Running = false;
      }
    }

    const Uint8* keys = SDL_GetKeyboardState(0);

    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
      YOffset += speed;
    } else if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
      YOffset -= speed;
    } else if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
      XOffset += speed;
    } else if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
      XOffset -= speed;
    }

    if (keys[SDL_SCANCODE_ESCAPE]) {
      Running = false;
    }

    SDLUpdatePixels(Buffer, XOffset, YOffset);
    SDLUpdateWindow(Window, Renderer, Buffer);
  }

  if (Buffer.Texture) { SDL_DestroyTexture(Buffer.Texture); }
  if (Buffer.Pixels) { free(Buffer.Pixels); }
  SDL_Quit();
}

