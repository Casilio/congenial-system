#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <x86intrin.h>

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
  AudioSettings.samples = 512;
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

struct sdl_sound_output {
  int SamplesPerSecond;
  int ToneHz;
  int16_t ToneVolume;
  uint32_t RunningSampleIndex;
  int WavePeriod;
  int BytesPerSample;
  int SecondaryBufferSize;
  float tSine;
  int LatencySampleCount;
};

void SDLFillSoundBuffer(sdl_sound_output *SoundOutput, int ByteToLock, int BytesToWrite) {
    void *Region1 = (uint8_t*)AudioRingBuffer.Data + ByteToLock;
    int Region1Size = BytesToWrite;

    if (Region1Size + ByteToLock > SoundOutput->SecondaryBufferSize) {
      Region1Size = SoundOutput->SecondaryBufferSize - ByteToLock;
    }

    void *Region2 = AudioRingBuffer.Data;
    int Region2Size = BytesToWrite - Region1Size;

    int Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
    int16_t *SampleOut = (int16_t*)Region1;

    for (int SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
      float SineValue = sin(SoundOutput->tSine);
      int16_t SampleValue = (int16_t)(SineValue * SoundOutput->ToneVolume);
      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;

      SoundOutput->tSine += 2.0f * M_PI * 1.0f / (float)SoundOutput->WavePeriod;
      ++SoundOutput->RunningSampleIndex;
    }

    int Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
    SampleOut = (int16_t *)Region2;

    for (int SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
      float SineValue = sin(SoundOutput->tSine);
      int16_t SampleValue = (int16_t)(SineValue * SoundOutput->ToneVolume);
      *SampleOut++ = SampleValue;
      *SampleOut++ = SampleValue;

      SoundOutput->tSine += 2.0f * M_PI * 1.0f / (float)SoundOutput->WavePeriod;
      ++SoundOutput->RunningSampleIndex;
    }
}

void update_tone(sdl_sound_output *SoundOutput, int NewTone) {
  if (NewTone <= 1 || NewTone > 4000) {
    return;
  }

  SoundOutput->ToneHz = NewTone;
  SoundOutput->WavePeriod = SoundOutput->SamplesPerSecond / SoundOutput->ToneHz;
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
  sdl_sound_output SoundOutput;
  SoundOutput.SamplesPerSecond = 48000;
  SoundOutput.ToneVolume = 3000;
  SoundOutput.RunningSampleIndex = 0;
  SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
  SoundOutput.tSine = 0.0f;
  SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
  SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
  update_tone(&SoundOutput, 256);

  // Open our audio device
  SDLInitAudio(48000, SoundOutput.SecondaryBufferSize);
  SDLFillSoundBuffer(&SoundOutput, 0, SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample);
  SDL_PauseAudio(0);

  uint64_t PerfCountFrequency = SDL_GetPerformanceFrequency();
  uint64_t LastCounter = SDL_GetPerformanceCounter();
  uint64_t LastCycleCount = _rdtsc();

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
      update_tone(&SoundOutput, SoundOutput.ToneHz + 1);
    }
    if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
      YOffset += speed;
      update_tone(&SoundOutput, SoundOutput.ToneHz - 1);
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

    // Sound output test
    SDL_LockAudio();
    int ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
    int TargetCursor = (AudioRingBuffer.PlayCursor + SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample) %
                        SoundOutput.SecondaryBufferSize;
    int BytesToWrite;
    if (ByteToLock > TargetCursor) {
      BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
      BytesToWrite += TargetCursor;
    } else {
      BytesToWrite = TargetCursor - ByteToLock;
    }
    SDL_UnlockAudio();
    SDLFillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);

    SDLUpdateWindow(Window, Renderer, Buffer);

    uint64_t EndCounter = SDL_GetPerformanceCounter();
    uint64_t CountElapsed = EndCounter - LastCounter;
    float MsPerFrme = (1000.0f * CountElapsed) / (float) PerfCountFrequency;
    float FPS = (float) PerfCountFrequency / (float) CountElapsed;
    uint64_t EndCycleCount = _rdtsc();
    uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
    float MegaCyclesPerFrame = (float)CyclesElapsed / (1000 * 1000);

    printf("ms/f: %.02f - fps: %.02f, mc/f - %.02f\n", MsPerFrme, FPS, MegaCyclesPerFrame);

    LastCounter = EndCounter;
    LastCycleCount = EndCycleCount;
  }

  if (Buffer.Texture) { SDL_DestroyTexture(Buffer.Texture); }
  if (Buffer.Pixels) { free(Buffer.Pixels); }
  SDL_Quit();
}

