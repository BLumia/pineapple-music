_**This is a not ready for use, toy project**_


## Note

> The Qt Multimedia APIs build upon the multimedia framework of the underlying platform. This can mean that support for various codecs or containers can vary between machines, depending on what the end user has installed.

Current implementation use all stuff from Qt which include the multimedia playback support, which means user may need to install codecs by themself. There is a link provided by QtWiki about [Qt 5.13 Multimedia Backends](https://wiki.qt.io/Qt_5.13_Multimedia_Backends), and here is a chart about some other things which might be helpful.

Platform|Built-in support|Probably useful 3rd-party codecs
---|---|---
Windows|[Supported Formats In DirectsShow](https://msdn.microsoft.com/en-us/library/windows/desktop/dd407173%28v=vs.85%29.aspx)|[Xiph.org: Directshow Filters for Ogg Vorbis, Speex, Theora, FLAC, and WebM](https://www.xiph.org/dshow/)
macOS|[Media formats supported by QuickTime Player](https://support.apple.com/en-us/HT201290)|Sorry, I don't know...
Unix/Linux|depends on [GStreamer](https://gstreamer.freedesktop.org/) plugins which user installed|[GStreamer Plug-ins: gst-plugins-base, gst-plugins-good, gst-plugins-ugly, gst-plugins-bad](https://gstreamer.freedesktop.org/documentation/additional/splitup.html?gi-language=c)

