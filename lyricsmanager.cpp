// SPDX-FileCopyrightText: 2024 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "lyricsmanager.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringConverter>

#ifdef HAVE_KCODECS
#include <KCharsets>
#include <KCodecs>
#include <KEncodingProber>
#endif

#ifdef USE_QTEXTCODEC
#include <QTextCodec>
#endif

Q_LOGGING_CATEGORY(lcLyrics, "pmusic.lyrics")
Q_LOGGING_CATEGORY(lcLyricsParser, "pmusic.lyrics.parser")

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
    QByteArray fileContent(file.readAll());
    QStringList lines;
#ifdef HAVE_KCODECS
    KEncodingProber prober(KEncodingProber::Universal);
    prober.feed(fileContent);
    QByteArray encoding(prober.encoding());
    qCDebug(lcLyrics) << "Detected encoding:" << QString(encoding) << "with confidence" << prober.confidence();
#ifdef USE_QTEXTCODEC
    qCDebug(lcLyrics) << "QTextCodec is used instead of QStringConverter.";
    QTextCodec *codec = QTextCodec::codecForName(encoding);
    if (codec) {
        lines = codec->toUnicode(fileContent).split('\n');
    } else {
        lines = QString(fileContent).split('\n');
        qCDebug(lcLyrics) << "No codec for the detected encoding. Available codecs are:" << QTextCodec::availableCodecs();
        qCDebug(lcLyrics) << "KCodecs offers these encodings:" << KCharsets::charsets()->availableEncodingNames();
    }
#else // NOT USE_QTEXTCODEC
    auto toUtf16 = QStringDecoder(encoding);
    // Don't use `QStringConverter::availableCodecs().contains(QString(encoding))` here, since the charset
    // encoding name might not match, e.g. GB18030 (from availableCodecs) != gb18030 (from KEncodingProber)
    if (toUtf16.isValid()) {
        QString decodedResult = toUtf16(fileContent);
        lines = decodedResult.split('\n');
    } else {
        qCDebug(lcLyrics) << "No codec for the detected encoding. Available codecs are:" << QStringConverter::availableCodecs();
        qCDebug(lcLyrics) << "KCodecs offers these encodings:" << KCharsets::charsets()->availableEncodingNames();
        lines = QString(fileContent).split('\n');
    }
#endif // USE_QTEXTCODEC
#else // NOT HAVE_KCODECS
    lines = QString(fileContent).split('\n');
#endif // HAVE_KCODECS
    file.close();

    // parse lyrics timestamp
    QRegularExpression tagRegex(R"regex(\[(ti|ar|al|au|length|by|offset|tool|re|ve|#):\s?([^\]]*)\]$)regex");
    QRegularExpression lrcRegex(R"regex(\[(\d{2,3}:\d{2}\.\d{2,3})\](.*))regex");
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
                    qCDebug(lcLyricsParser) << m_timeOffset;
                }
                qCDebug(lcLyricsParser) << "[tag]" << tagMatch.captured(1) << tagMatch.captured(2);
                continue;
            }
        }

        QList<int> timestamps;
        QString currentLrc;
        QRegularExpressionMatch match = lrcRegex.match(line);
        while (match.hasMatch()) {
            tagSectionPassed = true;
            timestamps.append(parseTimeToMilliseconds(match.captured(1)));
            currentLrc = match.captured(2);
            match = lrcRegex.match(currentLrc);
        }
        if (!timestamps.isEmpty()) {
            for (int timestamp : std::as_const(timestamps)) {
                m_lyricsMap.insert(timestamp, currentLrc);
                qCDebug(lcLyricsParser) << "[lrc]" << timestamp << currentLrc;
            }
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
    auto iter = std::find_if(m_timestampList.begin(), m_timestampList.end(), [&curTimeMs, this](int timestamp) -> bool {
        return (timestamp + m_timeOffset) > curTimeMs;
    });

    m_nextLyricsTime = iter == m_timestampList.end() ? totalTimeMs : *iter;
    if (iter != m_timestampList.begin()) {
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

int LyricsManager::parseTimeToMilliseconds(const QString &timeString)
{
    QRegularExpression timeRegex(R"((\d{2,3}):(\d{2})\.(\d{2,3}))");
    QRegularExpressionMatch match = timeRegex.match(timeString);

    if (match.hasMatch()) {
        int minutes = match.captured(1).toInt();
        int seconds = match.captured(2).toInt();
        int milliseconds = match.captured(3).toInt();

        return minutes * 60000 + seconds * 1000 + milliseconds;
    } else {
        qCWarning(lcLyricsParser) << "Invalid time format:" << timeString;
        return -1;
    }
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
