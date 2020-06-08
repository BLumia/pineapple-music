_**This is a not ready to use, toy project**_

Since **I** just need a simple player which *just works* right now, so I did many things in a dirty way. Don't feel so weird if you saw something I did in this project is using a bad approach.

## Note

> The Qt Multimedia APIs build upon the multimedia framework of the underlying platform. This can mean that support for various codecs or containers can vary between machines, depending on what the end user has installed.

Current implementation use all stuff from Qt which include the multimedia playback support, which means user may need to install codecs by themself. There is a link provided by QtWiki about [Qt 5.13 Multimedia Backends](https://wiki.qt.io/Qt_5.13_Multimedia_Backends), and here is a chart about some other things which might be helpful.

Platform|Built-in support|Probably useful 3rd-party codecs
---|---|---
Windows|[Supported Formats In DirectsShow](https://msdn.microsoft.com/en-us/library/windows/desktop/dd407173%28v=vs.85%29.aspx)|[Xiph.org: Directshow Filters for Ogg Vorbis, Speex, Theora, FLAC, and WebM](https://www.xiph.org/dshow/)
macOS|[Media formats supported by QuickTime Player](https://support.apple.com/en-us/HT201290)|Sorry, I don't know...
Unix/Linux|depends on [GStreamer](https://gstreamer.freedesktop.org/) plugins which user installed|[GStreamer Plug-ins: gst-plugins-base, gst-plugins-good, gst-plugins-ugly, gst-plugins-bad](https://gstreamer.freedesktop.org/documentation/additional/splitup.html?gi-language=c)

## Build

Current state, we need:

 - `cmake` as the build system.
 - `qt5` with `qt5-multimedia` since we use it for playback.
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

[Translate this project on Transifex!](https://www.transifex.com/blumia/pineapple-music/)

Feel free to open up an issue to request an new language to translate.

## About License

Since this is a toy repo, I don't spend much time about the license stuff. Currently this project use some assets and code from [ShadowPlayer](https://github.com/ShadowPower/ShadowPlayer), which have a very interesting license -- do whatever you want but cannot be used as homework -- obviously it's not a so called *free* license. I *may* do some license housecleaning works by replaceing the assets and code implementation when the code become reasonable, and the final codebase may probably released under MIT license.

Anyway here is a list of file which is in non-free state (with license: do whatever you want but cannot be used as homework):

 - All png images inside `icons` folder.
 - seekableslider.{h,cpp}

And something from ShadowPlayer but in other license:

 - {Flac,ID3v2}Pic.h : [AlbumCoverExtractor](https://github.com/ShadowPower/AlbumCoverExtractor), with [MIT License](https://github.com/ShadowPower/AlbumCoverExtractor/blob/master/LICENSE)

Also there are some source code which I copy-paste from Qt codebase, which released under BSD-3-Clause license by the Qt Company:

 - playlistmodel.{h,cpp}
