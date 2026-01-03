// SPDX-FileCopyrightText: 2026 Gary Wang <opensource@blumia.net>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QPropertyAnimation;
QT_END_NAMESPACE

class LyricsManager;
class LyricsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LyricsWidget(QWidget *parent = nullptr);

    void setLyricsManager(LyricsManager *mgr);
    void updatePosition(qint64 position);

private slots:
    void onLyricsLoaded(bool success);

private:
    QListWidget *m_listWidget;
    LyricsManager *m_lyricsManager = nullptr;
    int m_lastHighlightIndex = -1;
    QPropertyAnimation *m_scrollAnimation = nullptr;

    void updateLyrics();
    void highlightCurrentLine(int index);
};

