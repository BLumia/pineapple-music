// SPDX-FileCopyrightText: 2024 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "lrcbar.h"

#include "lyricsmanager.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWindow>

LrcBar::LrcBar(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , m_lrcMgr(new LyricsManager(this))
{
    m_font.setPointSize(30);
    m_font.setStyleStrategy(QFont::PreferAntialias);

    QSize windowSize(sizeHint());

    QFontMetrics fm(m_font);
    m_baseLinearGradient.setColorAt(0, QColor(20, 100, 200));
    m_baseLinearGradient.setColorAt(1, QColor(0, 80, 255));
    m_baseLinearGradient.setStart(0, (windowSize.height() - fm.height()) / 2);
    m_baseLinearGradient.setFinalStop(0, (windowSize.height() + fm.height()) / 2);
    m_maskLinearGradient.setColorAt(0, QColor(255, 128, 0));
    m_maskLinearGradient.setColorAt(0.5, QColor(255, 255, 0));
    m_maskLinearGradient.setColorAt(1, QColor(255, 128, 0));
    m_maskLinearGradient.setStart(0, (windowSize.height() - fm.height()) / 2);
    m_maskLinearGradient.setFinalStop(0, (windowSize.height() + fm.height()) / 2);

    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setGeometry(QRect(QPoint((qApp->primaryScreen()->geometry().width() - windowSize.width()) / 2,
                             qApp->primaryScreen()->geometry().height() - windowSize.height() - 50),
                      windowSize));
}

LrcBar::~LrcBar()
{

}

bool LrcBar::loadLyrics(QString filepath)
{
    m_currentTimeMs = 0;
    return m_lrcMgr->loadLyrics(filepath);
}

void LrcBar::playbackPositionChanged(int timestampMs, int totalTimeMs)
{
    if (!isVisible()) return;

    m_currentTimeMs = timestampMs;
    m_lrcMgr->updateCurrentTimeMs(timestampMs, totalTimeMs);
    update();
}

QSize LrcBar::sizeHint() const
{
    return QSize(1000, 88);
}

void LrcBar::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        window()->windowHandle()->startSystemMove();
        event->accept();
    }

    return QWidget::mouseMoveEvent(event);
}

void LrcBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (underMouse()) {
        painter.setBrush(QBrush(QColor(255, 255, 255, 120)));
        painter.setPen(Qt::NoPen);
        painter.drawRect(0, 0, width(), height());
    }
    painter.setFont(m_font);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QString curLrc(m_lrcMgr->lyrics().trimmed());
    if (curLrc.isEmpty()) {
        curLrc = m_lrcMgr->hasLyrics() ? tr("(Interlude...)")
                                       : QCoreApplication::translate("MainWindow", "Pineapple Music", nullptr);
    }

    QFontMetrics fm(m_font);
    int lrcWidth = fm.horizontalAdvance(curLrc);
    int maskWidth = lrcWidth * m_lrcMgr->maskPercent(m_currentTimeMs);
    int startOffsetX = 0;

    if (fm.horizontalAdvance(curLrc) < width()) {
        startOffsetX = (width() - lrcWidth) / 2;
    } else {
        if (maskWidth < width() / 2) {
            startOffsetX = 0;
        } else if (lrcWidth - maskWidth < width() / 2) {
            startOffsetX = width() - lrcWidth;
        } else {
            startOffsetX = 0 - (maskWidth - width() / 2);
        }
    }

    // shadow
    painter.setPen(QColor(0, 0, 0, 80));
    painter.drawText(startOffsetX + 2, 2, lrcWidth, this->height(), Qt::AlignVCenter, curLrc);
    // text itself
    painter.setPen(QPen(m_baseLinearGradient, 0));
    painter.drawText(startOffsetX, 0, lrcWidth, this->height(), Qt::AlignVCenter, curLrc);
    // mask
    painter.setPen(QPen(m_maskLinearGradient, 0));
    painter.drawText(startOffsetX, 0, maskWidth, this->height(), Qt::AlignVCenter, curLrc);
}

void LrcBar::enterEvent(QEnterEvent *)
{
    update();
}

void LrcBar::leaveEvent(QEvent *)
{
    update();
}
