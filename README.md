_**This is a not ready to use, toy project**_

Since **I** just need a simple player which *just works* right now, so I did many things in a dirty way. Don't feel so weird if you saw something I did in this project is using a bad approach.

## Build

Current state, we need:

 - `cmake` as the build system.
 - `qt6` with `qt6-multimedia` since we use it for playback.
 - `taglib` to get the audio file properties.
 - `pkg-config` to find the installed taglib.

Then we can build it with any proper c++ compiler like g++ or msvc.

### Linux

Just normal build process as other program. Nothing special ;)

### Windows

Install the depts manually is a nightmare. I use [KDE Craft](https://community.kde.org/Craft) but MSYS2 should also works. FYI currently this project is not intended to works under Windows (it should works and I also did some simple test though).

### macOS

I don't have a mac, so no support at all.

## Help Translation!

TODO: move to Codeberg's Weblate.

## About License

Since this is a toy repo, I don't spend much time about the license stuff. Currently this project use some assets and code from [ShadowPlayer](https://github.com/ShadowPower/ShadowPlayer), which have a very interesting license -- do whatever you want but cannot be used as homework -- obviously it's not a so called *free* license. I *may* do some license housecleaning works by replaceing the assets and code implementation when the code become reasonable, and the final codebase may probably released under MIT license.

Anyway here is a list of file which is in non-free state (with license: do whatever you want but cannot be used as homework):

 - All png images inside `icons` folder.
 - seekableslider.{h,cpp}
