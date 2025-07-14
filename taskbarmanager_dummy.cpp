// SPDX-FileCopyrightText: 2025 Gary Wang <opensource@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "taskbarmanager.h"

#include <QDebug>

class TaskBarManagerPrivate {};

TaskBarManager::TaskBarManager(QObject *parent)
    : QObject(parent)
{
}

TaskBarManager::~TaskBarManager() = default;

bool TaskBarManager::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *)
{
    return false;
}

QMediaPlayer::PlaybackState TaskBarManager::playbackState() const
{
    return QMediaPlayer::StoppedState;
}

bool TaskBarManager::showProgress() const
{
    return false;
}

qulonglong TaskBarManager::progressMaximum() const
{
    return 100;
}

qulonglong TaskBarManager::progressValue() const
{
    return 50;
}

bool TaskBarManager::canSkipBackward() const
{
    return false;
}

bool TaskBarManager::canSkipForward() const
{
    return false;
}

bool TaskBarManager::canTogglePlayback() const
{
    return false;
}

void TaskBarManager::setWinId(WId winId)
{
}

void TaskBarManager::setPlaybackState(const QMediaPlayer::PlaybackState newPlaybackState)
{
    Q_UNUSED(newPlaybackState);
}

void TaskBarManager::setShowProgress(const bool showProgress)
{
    Q_UNUSED(showProgress);
}

void TaskBarManager::setProgressMaximum(const qlonglong newMaximum)
{
    Q_UNUSED(newMaximum);
}

void TaskBarManager::setProgressValue(const qlonglong newValue)
{
    Q_UNUSED(newValue);
}

void TaskBarManager::setCanSkipBackward(const bool canSkip)
{
    Q_UNUSED(canSkip);
}

void TaskBarManager::setCanSkipForward(const bool canSkip)
{
    Q_UNUSED(canSkip);
}

void TaskBarManager::setCanTogglePlayback(const bool canToggle)
{
    Q_UNUSED(canToggle);
}

#include "moc_taskbarmanager.cpp"
