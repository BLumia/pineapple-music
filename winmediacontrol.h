// SPDX-FileCopyrightText: 2026 Gary Wang <opensource@blumia.net>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QObject>
#include <QString>
#include <QImage>

class WinMediaControlPrivate;

class WinMediaControl : public QObject
{
    Q_OBJECT

public:
    explicit WinMediaControl(QObject* parent = nullptr);
    ~WinMediaControl();

    void initialize(void* hwnd);
    void shutdown();

    void setMetadata(const QString& title, const QString& artist,
                     const QString& album);
    void setCoverArt(const QImage& coverArt);
    void setPlaybackState(int state);
    void setPosition(qint64 positionMs, qint64 durationMs);
    void setControlsEnabled(bool play, bool pause, bool stop,
                            bool next, bool previous);

signals:
    void playRequested();
    void pauseRequested();
    void stopRequested();
    void nextRequested();
    void previousRequested();
    void seekRequested(qint64 positionMs);

private:
    WinMediaControlPrivate* d = nullptr;
};
