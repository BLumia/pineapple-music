// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "playbackprogressindicator.h"

#include <QPainter>
#include <QPainterPath>

PlaybackProgressIndicator::PlaybackProgressIndicator(QWidget *parent) :
    QWidget(parent)
{
}

void PlaybackProgressIndicator::setPosition(qint64 pos)
{
    m_position = pos;
    emit positionChanged(m_position);
}

void PlaybackProgressIndicator::setDuration(qint64 dur)
{
    m_duration = dur;
    emit durationChanged(m_duration);
}

void PlaybackProgressIndicator::setChapters(QList<std::pair<qint64, QString> > chapters)
{
    m_chapterModel.clear();
    for (const std::pair<qint64, QString> & chapter : chapters) {
        QStandardItem * chapterItem = new QStandardItem(chapter.second);
        chapterItem->setData(chapter.first, StartTimeMsRole);
        m_chapterModel.appendRow(chapterItem);
    }
    update();
}

void PlaybackProgressIndicator::paintEvent(QPaintEvent *event)
{
    constexpr int progressBarHeight = 6;
    constexpr QColor activeColor = QColor(85, 170, 0);
    const QPointF topLeft(0, height() / 2.0 - progressBarHeight / 2.0);
    const QSizeF barSize(width(), progressBarHeight);

    const float currentProgress = m_duration <= 0 ? 0 : (m_seekingPosition >= 0 ? m_seekingPosition : m_position) / (float)m_duration;
    const QSizeF progressSize(width() * currentProgress, progressBarHeight);

    QPainterPath theProgress;
    theProgress.addRoundedRect(QRectF(topLeft, progressSize), progressBarHeight / 2, progressBarHeight / 2);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.save();

    // the bar itself
    painter.setPen(Qt::gray);
    painter.drawRoundedRect(QRectF(topLeft, barSize), progressBarHeight / 2, progressBarHeight / 2);
    painter.fillPath(theProgress, activeColor);

    // progress
    painter.setPen(activeColor);
    painter.drawPath(theProgress);

    // chapter markers
    if (m_duration > 0) {
        painter.setPen(Qt::lightGray);
        for (int i = 0; i < m_chapterModel.rowCount(); i++) {
            qint64 chapterStartTime = m_chapterModel.item(i)->data(StartTimeMsRole).toInt();
            if (chapterStartTime > m_duration) break;
            float chapterPercent = chapterStartTime / (float)m_duration;
            float chapterPosX = width() * chapterPercent;
            painter.drawLine(topLeft + QPoint(chapterPosX, 0),
                             topLeft + QPoint(chapterPosX, progressBarHeight));
        }
    }

    painter.restore();
}

void PlaybackProgressIndicator::mousePressEvent(QMouseEvent *event)
{
    if (m_duration > 0) {
        event->accept();
    } else {
        return QWidget::mousePressEvent(event);
    }
}

void PlaybackProgressIndicator::mouseMoveEvent(QMouseEvent *event)
{
    if (m_duration > 0) {
        m_seekingPosition = event->position().x() * m_duration / width();
        if (m_seekOnMove) {
            emit seekingRequested(m_seekingPosition);
        }
        update();
    }
    return QWidget::mouseMoveEvent(event);
}

void PlaybackProgressIndicator::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_duration > 0) {
        int seekingPosition = event->position().x() * m_duration / width();
        m_seekingPosition = -1;
        emit seekingRequested(seekingPosition);
    }
    update();
    return QWidget::mouseReleaseEvent(event);
}
