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

struct sdl_audio_ring_buffer {
  int Size;
  int WriteCursor;
  int PlayCursor;
  void *Data;
};

sdl_audio_ring_buffer AudioRingBuffer;

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

void SDLAudioCallback(void *UserData, uint8_t *AudioData, int Length) {
  sdl_audio_ring_buffer *RingBuffer = (sdl_audio_ring_buffer *)UserData;

  int Region1Size = Length;
  int Region2Size = 0;
  if (RingBuffer->PlayCursor + Length > RingBuffer->Size) {
    Region1Size = RingBuffer->Size -  RingBuffer->PlayCursor;
    Region2Size = Length - Region1Size;
  }
  memcpy(AudioData, (uint8_t*)(RingBuffer->Data) + RingBuffer->PlayCursor, Region1Size);
  memcpy(&AudioData[Region1Size], RingBuffer->Data, Region2Size);
  RingBuffer->PlayCursor = (RingBuffer->PlayCursor + Length) % RingBuffer->Size;
  RingBuffer->WriteCursor = (RingBuffer->PlayCursor + 2048) % RingBuffer->Size;
}

void SDLInitAudio(int32_t SamplePerSecond, int32_t BufferSize) {
  SDL_AudioSpec AudioSettings = {0};

  AudioSettings.freq = SamplePerSecond;
  AudioSettings.format = AUDIO_S16LSB;
  AudioSettings.channels = 2;
  AudioSettings.samples = 1024;
  AudioSettings.callback = &SDLAudioCallback;
  AudioSettings.userdata = &AudioRingBuffer;

  AudioRingBuffer.Size = BufferSize;
  AudioRingBuffer.Data = malloc(BufferSize);
  AudioRingBuffer.PlayCursor = AudioRingBuffer.WriteCursor = 0;

  SDL_OpenAudio(&AudioSettings, 0);

  printf("Initialised an audio device at frequency %d Hz, %d Channels, buffer size %d\n",
         AudioSettings.freq, AudioSettings.channels, AudioSettings.samples);

  if (AudioSettings.format != AUDIO_S16LSB) {
    printf("DIDN'T GET AUDIO FORMAT");
    SDL_CloseAudio();
  }
}

int main() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
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

  // Sound Test
  int SamplesPerSecond = 48000;
  int ToneHz = 256;
  int16_t ToneVolume = 3000;
  uint32_t RunningSampleIndex = 0;
  int SquareWavePeriod = SamplesPerSecond / ToneHz;
  int HalfSquareWavePeriod = SquareWavePeriod / 2;
  int BytesPerSample = sizeof(int16_t) * 2;
  int SecondaryBufferSize = SamplesPerSecond * BytesPerSample;

  // Open our audio device
  SDLInitAudio(48000, SecondaryBufferSize);
  bool SoundIsPlaying = false;

  while(Running) {
    SDL_Event Event;

    while(SDL_PollEvent(&Event)) {
      if (HandleEvent(&Event)) {
        Running = false;
      }
    }

    const Uint8* keys = SDL_GetKeyboardState(0);

    if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
      YOffset -= speed;
    }
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
      YOffset += speed;
    }
    if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
      XOffset += speed;
    }
    if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
      XOffset -= speed;
    }

    if (keys[SDL_SCANCODE_ESCAPE]) {
      Running = false;
    }

    SDLUpdatePixels(Buffer, XOffset, YOffset);
    SDLUpdateWindow(Window, Renderer, Buffer);

    // Sound output test
    SDL_LockAudio();
    int BytesToLock = RunningSampleIndex * BytesPerSample % SecondaryBufferSize;
    int BytesToWrite;
    if (BytesToLock == AudioRingBuffer.PlayCursor) {
      BytesToWrite = SecondaryBufferSize;
    } else if (BytesToLock > AudioRingBuffer.PlayCursor) {
      BytesToWrite = (SecondaryBufferSize - BytesToLock);
      BytesToWrite += AudioRingBuffer.PlayCursor;
    } else {
      BytesToWrite = AudioRingBuffer.PlayCursor - BytesToLock;
    }

    void *Region1 = (uint8_t*)AudioRingBuffer.Data + BytesToLock;
    int Region1Size = BytesToWrite;

    if (Region1Size + BytesToLock > SecondaryBufferSize) {
      Region1Size = SecondaryBufferSize - BytesToLock;
    }

    void *Region2 = AudioRingBuffer.Data;
    int Region2Size = BytesToWrite - Region1Size;
    SDL_UnlockAudio();

    int Region1SampleCount = Region1Size / BytesPerSample;
    int16_t *SampleOut = (int16_t*)Region1;

    for (int SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
      int16_t SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;

      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;
    }

    int Region2SampleCount = Region2Size / BytesPerSample;
    SampleOut = (int16_t *)Region2;

    for (int SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
      int16_t SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume : -ToneVolume;

      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;
    }

    if (!SoundIsPlaying) {
      SDL_PauseAudio(0);
      SoundIsPlaying = true;
    }
  }

  if (Buffer.Texture) { SDL_DestroyTexture(Buffer.Texture); }
  if (Buffer.Pixels) { free(Buffer.Pixels); }
  SDL_Quit();
}

