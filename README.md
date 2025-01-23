## Read Before Use

Since **I** just need a simple player which *just works* right now, so I did many things in a dirty way. Don't feel so weird if you saw something I did in this project is using a bad approach.

### Features

We have the following features:

- [Sidecar](https://en.wikipedia.org/wiki/Sidecar_file) lyrics file (`.lrc`) support with an optional desktop lyrics bar widget
- Sidecar chapter file support
  - [YouTube-style chapter](https://support.google.com/youtube/answer/9884579) saved to a plain text file with `.chp` suffix
  - PotPlayer `.pbf` file, `[Bookmark]`s as chapters
- Auto-load all audio files in the same folder of the file that you attempted to play, into a playlist

These features are not available, some of them are TBD and others are not planned:

- File format support will be limited by the [FFmpeg version that Qt 6 uses](https://doc.qt.io/qt-6/qtmultimedia-attribution-ffmpeg.html).
  - ...which if you use Qt's official binary, only contains the LGPLv2.1+ part. (already good enough, tho)
- No music library management support and there won't be one!
  - It'll auto-load music files in the same folder of the file that you attempted to play, so organize your music files on a folder-basis.
- Limited system integration:
  - No [SMTC](https://learn.microsoft.com/en-us/uwp/api/windows.media.systemmediatransportcontrols) support under Windows for now
  - No [MPRIS](https://www.freedesktop.org/wiki/Specifications/mpris-spec/) support under Linux desktop for now
  - No "playback progress on taskbar icon" and "taskbar thumbnail buttons" support whatever on Windows or Linux desktop for now
- Limited lyrics (`.lrc`) loading support:
  - Currently no `.tlrc` (for translated lyrics) or `.rlrc` (for romanized lyrics) support.
  - Multi-line lyrics and duplicated timestamps are not supported
  - Extensions (Walaoke and A2 extension) are not supported

## Build

Current state, we need:

 - `cmake` as the build system.
 - `qt6` with `qt6-multimedia` since we use it for playback.
 - `taglib` to get the audio file properties.
 - `kissfft` for FFT support (will be downloaded at configure-time by `cmake`).

Then we can build it with any proper c++ compiler like g++ or msvc.

Building it just requires normal cmake building steps:

```shell
$ cmake -Bbuild
$ cmake --build build
```

## Help Translation!

[Translate this project on Codeberg's Weblate!](https://translate.codeberg.org/projects/pineapple-apps/pineapple-music/)

## About License

Pineapple Music as a whole is licensed under MIT license. Individual files may have a different, but compatible license.

All *png* images inside `icons` folder are originally created by [@ShadowPower](https://github.com/ShadowPower/) for [ShadowPlayer](https://github.com/ShadowPower/ShadowPlayer). These images are licensed under [CC0](https://creativecommons.org/publicdomain/zero/1.0/legalcode) license, grant by the original author.
