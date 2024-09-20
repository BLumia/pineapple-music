// SPDX-FileCopyrightText: 2024 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "lyricsmanager.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

LyricsManager::LyricsManager(QObject *parent)
    : QObject(parent)
{

}

LyricsManager::~LyricsManager()
{

}

bool LyricsManager::loadLyrics(QString filepath)
{
    // reset state
    reset();

    // check and load file
    QFileInfo fileInfo(filepath);
    if (!filepath.endsWith(".lrc", Qt::CaseInsensitive)) {
        fileInfo.setFile(fileInfo.dir().filePath(fileInfo.completeBaseName() + ".lrc"));
    }
    if (!fileInfo.exists()) return false;

    QFile file(fileInfo.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream stream(&file);
    QStringList lines = QString(stream.readAll()).split('\n');
    file.close();

    // parse lyrics timestamp
    QRegularExpression tagRegex(R"regex(\[(ti|ar|al|au|length|by|offset|tool|re|ve|#):\s?([^\]]*)\]$)regex");
    QRegularExpression lrcRegex(R"regex(\[(\d{2,3}:\d{2}\.\d{2})\](.*))regex");
    bool tagSectionPassed = false;

    for (QString line : std::as_const(lines)) {
        line = line.trimmed();
        if (line.isEmpty()) continue;

        if (!tagSectionPassed) {
            QRegularExpressionMatch tagMatch = tagRegex.match(line);
            if (tagMatch.hasMatch()) {
                QString tag(tagMatch.captured(1));
                if (tag == QLatin1String("offset")) {
                    // The value is prefixed with either + or -, with + causing lyrics to appear sooner
                    m_timeOffset = -tagMatch.captured(2).toInt();
                    qDebug() << m_timeOffset;
                }
                qDebug() << "[tag]" << tagMatch.captured(1) << tagMatch.captured(2);
                continue;
            }
        }

        QRegularExpressionMatch match = lrcRegex.match(line);
        if (match.hasMatch()) {
            tagSectionPassed = true;
            QTime timestamp(QTime::fromString(match.captured(1), "m:s.zz"));
            m_lyricsMap.insert(timestamp.msecsSinceStartOfDay(), match.captured(2));
            qDebug() << "[lrc]" << match.captured(1) << match.captured(2);
        }
    }
    if (!m_lyricsMap.isEmpty()) {
        m_timestampList = m_lyricsMap.keys();
        std::sort(m_timestampList.begin(), m_timestampList.end());
        return true;
    }

    return false;
}

bool LyricsManager::hasLyrics() const
{
    return !m_lyricsMap.isEmpty();
}

void LyricsManager::updateCurrentTimeMs(int curTimeMs, int totalTimeMs)
{
    if (!hasLyrics()) return;

    // TODO: we don't need to find from the top everytime the time is updated
    auto iter = std::find_if(m_timestampList.begin(), m_timestampList.end() + 1, [&curTimeMs, this](int timestamp) -> bool {
        return (timestamp + m_timeOffset) > curTimeMs;
    });

    m_nextLyricsTime = (iter == m_timestampList.end() + 1) ? totalTimeMs : *iter;
    if (iter != m_timestampList.begin() && iter != (m_timestampList.end() + 1)) {
        iter--;
    }
    m_currentLyricsTime = *iter;
}

QString LyricsManager::lyrics(int lineOffset) const
{
    if (!hasLyrics()) return QString();

    int index = m_timestampList.indexOf(m_currentLyricsTime) + lineOffset;
    if (index >= 0 && index < m_timestampList.count()) {
        int timestamp = m_timestampList.at(index);
        return m_lyricsMap.value(timestamp);
    } else {
        return QString();
    }
}

double LyricsManager::maskPercent(int curTimeMs)
{
    if (!hasLyrics()) return 0;
    if (curTimeMs <= currentLyricsTime()) return 0;
    if (curTimeMs >= nextLyricsTime()) return 1;
    if (m_nextLyricsTime == currentLyricsTime()) return 1;

    return (double)(curTimeMs - currentLyricsTime()) / (m_nextLyricsTime - m_currentLyricsTime);
}

void LyricsManager::reset()
{
    m_currentLyricsTime = 0;
    m_nextLyricsTime = 0;
    m_timeOffset = 0;
    m_lyricsMap.clear();
    m_timestampList.clear();
}

int LyricsManager::currentLyricsTime() const
{
    return m_currentLyricsTime + m_timeOffset;
}

int LyricsManager::nextLyricsTime() const
{
    return m_nextLyricsTime + m_timeOffset;
}
