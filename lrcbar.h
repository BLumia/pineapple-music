// SPDX-FileCopyrightText: 2024 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QLinearGradient>
#include <QMediaPlayer>
#include <QWidget>

class LyricsManager;
class LrcBar : public QWidget
{
    Q_OBJECT
public:
    explicit LrcBar(QWidget *parent);
    ~LrcBar();

    bool loadLyrics(QString filepath);
    void playbackPositionChanged(int timestampMs, int totalTimeMs);

protected:
    QSize sizeHint() const override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    int m_currentTimeMs = 0;
    LyricsManager * m_lrcMgr;
    QFont m_font;
    QLinearGradient m_baseLinearGradient;
    QLinearGradient m_maskLinearGradient;
    bool m_underMouse = false;
};
