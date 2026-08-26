# libsimplewebm

A small WebM demuxer & decoder library. It uses Google's [libwebm](https://chromium.googlesource.com/webm/libwebm) for WebM/Matroska parsing, [libvpx](https://chromium.googlesource.com/webm/libvpx) for VP8/VP9 video decoding & [libopus](https://opus-codec.org/) / [libvorbis](https://xiph.org/vorbis/) for audio decoding.

## Features

- WebM/Matroska demuxing
- VP8 & VP9 video decoding
- Opus & Vorbis audio decoding

## Requirements

- CMake 4.0+
- C++11 or above compiler
- libvpx
- libopus
- libvorbis
- pkg-config

> [!NOTE]
> libwebm's required sources are included in the repo.

## Building

```sh
cmake -S . -B build
cmake --build build
```

## Example

The example takes a WebM file & decodes its video & audio streams:

```sh
./build/libsimplewebm_example video.webm
```

## Architecture
```cpp
                 WebM / Matroska file
                           │
                           ▼
                    ┌─────────────┐
                    │  libwebm    │
                    │  mkvparser  │
                    └──────┬──────┘
                           │
                           ▼
                    ┌─────────────┐
                    │ WebMDemuxer │
                    └──────┬──────┘
                           │
                 ┌─────────┴─────────┐
                 ▼                   ▼
          ┌────────────┐      ┌───────────────┐
          │ VPXDecoder │      │ OpusVorbis    │
          │            │      │ Decoder       │
          └─────┬──────┘      └───────┬───────┘
                │                     │
                ▼                     ▼
             VP8/VP9              Opus/Vorbis
              frames                PCM audio
```