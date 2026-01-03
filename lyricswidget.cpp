// SPDX-FileCopyrightText: 2026 Gary Wang <opensource@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "lyricswidget.h"
#include "lyricsmanager.h"

#include <QListWidget>
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QListWidgetItem>
#include <QFont>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QEasingCurve>

LyricsWidget::LyricsWidget(QWidget *parent)
    : QWidget(parent)
    , m_listWidget(new QListWidget(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_listWidget);

    m_listWidget->setFrameShape(QFrame::NoFrame);
    m_listWidget->setStyleSheet("background: transparent;");
    m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Hide scrollbar for cleaner look? Or Keep it? User didn't specify. Hiding it looks more like "Lyrics Mode"
    // Enable word wrap
    m_listWidget->setWordWrap(true);
    m_listWidget->setTextElideMode(Qt::ElideNone);
    m_listWidget->setSelectionMode(QAbstractItemView::NoSelection);
    m_listWidget->setFocusPolicy(Qt::NoFocus);
}

void LyricsWidget::setLyricsManager(LyricsManager *mgr)
{
    if (m_lyricsManager) {
        disconnect(m_lyricsManager, nullptr, this, nullptr);
    }
    m_lyricsManager = mgr;
    if (m_lyricsManager) {
        connect(m_lyricsManager, &LyricsManager::lyricsLoaded, this, &LyricsWidget::onLyricsLoaded);
        // Load initial state if any
        onLyricsLoaded(m_lyricsManager->hasLyrics());
    }
}

void LyricsWidget::updatePosition(qint64 position)
{
    if (!m_lyricsManager || !m_lyricsManager->hasLyrics() || !isVisible()) return;

    // Note: rely on LyricsManager::updateCurrentTimeMs()
    int index = m_lyricsManager->currentLineIndex();

    if (index != m_lastHighlightIndex) {
        highlightCurrentLine(index);
    }
}

void LyricsWidget::onLyricsLoaded(bool success)
{
    m_listWidget->clear();
    m_lastHighlightIndex = -1;

    if (!success || !m_lyricsManager) return;

    const QList<int>& timestamps = m_lyricsManager->timestamps();
    const QHash<int, QString>& lyricsMap = m_lyricsManager->lyricsMap();

    for (int timestamp : timestamps) {
        QListWidgetItem *item = new QListWidgetItem(lyricsMap.value(timestamp));
        item->setTextAlignment(Qt::AlignCenter);

        // Default style
        QFont font = item->font();
        font.setPointSize(10);
        item->setFont(font);
        item->setForeground(QColor(255, 255, 255, 150)); // Dimmed

        m_listWidget->addItem(item);
    }
}

void LyricsWidget::highlightCurrentLine(int index)
{
    if (index < 0 || index >= m_listWidget->count()) return;

    // Reset old highlight
    if (m_lastHighlightIndex >= 0 && m_lastHighlightIndex < m_listWidget->count()) {
        QListWidgetItem *oldItem = m_listWidget->item(m_lastHighlightIndex);
        QFont font = oldItem->font();
        font.setPointSize(10);
        font.setBold(false);
        oldItem->setFont(font);
        oldItem->setForeground(QColor(255, 255, 255, 150));
    }

    // Set new highlight
    QListWidgetItem *newItem = m_listWidget->item(index);
    QFont font = newItem->font();
    font.setPointSize(14);
    font.setBold(true);
    newItem->setFont(font);
    newItem->setForeground(QColor(255, 255, 255, 255));

    // Smooth scroll
    QScrollBar *vBar = m_listWidget->verticalScrollBar();
    int startScroll = vBar->value();

    m_listWidget->scrollToItem(newItem, QAbstractItemView::PositionAtCenter);
    int endScroll = vBar->value();

    // Restore and animate
    if (startScroll != endScroll) {
        vBar->setValue(startScroll);

        if (!m_scrollAnimation) {
            m_scrollAnimation = new QPropertyAnimation(vBar, "value", this);
            m_scrollAnimation->setDuration(400);
            m_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
        }
        m_scrollAnimation->stop();
        m_scrollAnimation->setStartValue(startScroll);
        m_scrollAnimation->setEndValue(endScroll);
        m_scrollAnimation->start();
    }

    m_lastHighlightIndex = index;
}
