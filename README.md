## Read Before Use

Since **I** just need a simple player which *just works* right now, so I did many things in a dirty way. Don't feel so weird if you saw something I did in this project is using a bad approach.

### Feature Notice

- File format support will be limited by the [FFmpeg version that Qt 6 uses](https://doc.qt.io/qt-6/qtmultimedia-attribution-ffmpeg.html).
  - ...which if you use Qt's official binary, only contains the LGPLv2.1+ part. (already good enough, tho)
- No music library management support and there won't be one!

## Build

Current state, we need:

 - `cmake` as the build system.
 - `qt6` with `qt6-multimedia` since we use it for playback.
 - `taglib` to get the audio file properties.
 - `pkg-config` to find the installed taglib.

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
