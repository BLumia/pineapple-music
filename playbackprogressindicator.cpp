// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "playbackprogressindicator.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>

#ifdef HAVE_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/time.h> // Contains AV_TIME_BASE and AV_TIME_BASE_Q
} // extern "C"
#endif // HAVE_FFMPEG

PlaybackProgressIndicator::PlaybackProgressIndicator(QWidget *parent) :
    QWidget(parent)
{
}

QModelIndex PlaybackProgressIndicator::currentChapterItem() const
{
    int currentChapterIndex = -1;

    for (int i = 0; i < m_chapterModel.rowCount(); i++) {
        QStandardItem* timeItem = m_chapterModel.item(i, 0);
        qint64 chapterStartTime = timeItem->data(PlaybackProgressIndicator::StartTimeMsRole).toLongLong();

        if (m_position >= chapterStartTime) {
            currentChapterIndex = i;
        } else {
            break;
        }
    }

    if (currentChapterIndex >= 0) {
        return m_chapterModel.index(currentChapterIndex, 0);
    }

    return {};
}

QString PlaybackProgressIndicator::currentChapterName() const
{
    const QModelIndex timeIndex(currentChapterItem());
    if (timeIndex.isValid()) {
        return m_chapterModel.item(timeIndex.row(), 1)->text();
    }
    return {};
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

QString PlaybackProgressIndicator::formatTime(qint64 milliseconds)
{
    QTime duaTime(QTime::fromMSecsSinceStartOfDay(milliseconds));
    if (duaTime.hour() > 0) {
        return duaTime.toString("h:mm:ss");
    } else {
        return duaTime.toString("m:ss");
    }
}

void PlaybackProgressIndicator::setChapters(QList<std::pair<qint64, QString> > chapters)
{
    m_chapterModel.clear();

    m_chapterModel.setHorizontalHeaderLabels(QStringList() << tr("Time") << tr("Chapter Name"));

    for (const std::pair<qint64, QString> & chapter : chapters) {
        QList<QStandardItem*> row;

        QStandardItem * timeItem = new QStandardItem(formatTime(chapter.first));
        timeItem->setData(chapter.first, StartTimeMsRole);
        row.append(timeItem);

        QStandardItem * chapterItem = new QStandardItem(chapter.second);
        chapterItem->setData(chapter.first, StartTimeMsRole);
        row.append(chapterItem);

        m_chapterModel.appendRow(row);
    }
    update();
}

QList<std::pair<qint64, QString> > PlaybackProgressIndicator::tryLoadChapters(const QString &filePath)
{
    auto chapters = tryLoadSidecarChapterFile(filePath);
    if (chapters.size() == 0) {
        chapters = tryLoadChaptersFromMetadata(filePath);
    }
    return chapters;
}

QList<std::pair<qint64, QString> > PlaybackProgressIndicator::tryLoadSidecarChapterFile(const QString &filePath)
{
    if (filePath.endsWith(".chp", Qt::CaseInsensitive)) {
        return parseCHPChapterFile(filePath);
    } else if (filePath.endsWith(".pbf", Qt::CaseInsensitive)) {
        return parsePBFChapterFile(filePath);
    }

    QFileInfo fileInfo(filePath);
    fileInfo.setFile(fileInfo.dir().filePath(fileInfo.completeBaseName() + ".chp"));
    if (fileInfo.exists()) {
        return parseCHPChapterFile(fileInfo.absoluteFilePath());
    }
    fileInfo.setFile(fileInfo.dir().filePath(fileInfo.completeBaseName() + ".pbf"));
    if (fileInfo.exists()) {
        return parsePBFChapterFile(fileInfo.absoluteFilePath());
    }
    fileInfo.setFile(filePath + ".chp");
    if (fileInfo.exists()) {
        return parseCHPChapterFile(fileInfo.absoluteFilePath());
    }
    return {};
}

#ifdef HAVE_FFMPEG

// Helper function to convert FFmpeg time (in time_base units) to milliseconds
qint64 convertTimestampToMilliseconds(int64_t timestamp, AVRational time_base) {
    // Convert to seconds first, then to milliseconds and cast to qint64
    return static_cast<qint64>((double)timestamp * av_q2d(time_base) * 1000.0);
}

// Helper function to print FFmpeg errors
void printFFmpegError(int errnum) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, errbuf, sizeof(errbuf));
    qCritical() << "FFmpeg error:" << errbuf;
}

#endif // HAVE_FFMPEG

QList<std::pair<qint64, QString> > PlaybackProgressIndicator::tryLoadChaptersFromMetadata(const QString &filePath)
{
#ifdef HAVE_FFMPEG
    if (!QFile::exists(filePath)) {
        qCritical() << "Error: File not found" << filePath;
        return {};
    }

    AVFormatContext* format_ctx = nullptr; // FFmpeg format context
    int ret = 0; // Return value for FFmpeg functions

    qInfo() << "Attempting to open file:" << filePath;

    // Open the input file and read the header.
    // The last two arguments (AVInputFormat*, AVDictionary**) are optional.
    // Passing nullptr for them means FFmpeg will try to guess the format
    // and no options will be passed to the demuxer.
    ret = avformat_open_input(&format_ctx, filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0) {
        qCritical() << "Could not open input file:" << filePath;
        printFFmpegError(ret);
        return {};
    }
    qInfo() << "File opened successfully.";

    // Read stream information from the file.
    // This populates format_ctx->streams and other metadata, including chapters.
    ret = avformat_find_stream_info(format_ctx, nullptr);
    if (ret < 0) {
        qCritical() << "Could not find stream information for file:" << filePath;
        printFFmpegError(ret);
        avformat_close_input(&format_ctx); // Close the context before returning
        return {};
    }
    qInfo() << "Stream information found.";

    QList<std::pair<qint64, QString>> chapterList;

    // Check if there are any chapters
    if (format_ctx->nb_chapters == 0) {
        qInfo() << "No chapters found in file:" << filePath;
    } else {
        qInfo() << "Found" << format_ctx->nb_chapters << "chapters.";
        // Iterate through each chapter
        for (unsigned int i = 0; i < format_ctx->nb_chapters; ++i) {
            AVChapter* chapter = format_ctx->chapters[i];

            // Chapter timestamps are typically in AV_TIME_BASE units by default
            // unless the chapter itself has a specific time_base.
            // For simplicity and common cases, we use AV_TIME_BASE_Q.
            qint64 start_ms = convertTimestampToMilliseconds(chapter->start, chapter->time_base);

            // Get the chapter title from its metadata.
            // av_dict_get(dictionary, key, prev, flags)
            // prev is used for iterating through multiple entries with the same key,
            // we want the first one so we pass nullptr. flags=0 for case-insensitive.
            AVDictionaryEntry* title_tag = av_dict_get(chapter->metadata, "title", nullptr, 0);
            QString chapter_title = (title_tag && title_tag->value) ? QString::fromUtf8(title_tag->value) : "Untitled Chapter";

            chapterList.append(std::make_pair(start_ms, chapter_title));
        }
    }

    // Close the input file.
    // This also frees the format_ctx and associated data.
    avformat_close_input(&format_ctx);
    qInfo() << "File closed.";

    return chapterList;
#else
    qInfo() << "FFmpeg not found during build.";
    return {};
#endif // HAVE_FFMPEG
}

QList<std::pair<qint64, QString> > PlaybackProgressIndicator::parseCHPChapterFile(const QString &filePath)
{
    QList<std::pair<qint64, QString>> chapters;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return chapters;
    }

    QTextStream in(&file);
    QRegularExpression timeRegex(R"((\d{1,2}):(\d{2})(?::(\d{2}))?(?:\.(\d{1,3}))?)");

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QRegularExpressionMatch match = timeRegex.match(line);
        if (match.hasMatch()) {
            int hours = match.capturedView(3).isEmpty() ? 0 : match.capturedView(1).toInt();
            int minutes = match.capturedView(3).isEmpty() ? match.capturedView(1).toInt() : match.capturedView(2).toInt();
            int seconds = match.capturedView(3).isEmpty() ? match.capturedView(2).toInt() : match.capturedView(3).toInt();
            int milliseconds = 0;

            QStringView millisecondsStr(match.capturedView(4));
            if (!millisecondsStr.isEmpty()) {
                milliseconds = millisecondsStr.toInt() * pow(10, 3 - millisecondsStr.length());
            }

            qint64 totalMilliseconds = (hours * 3600 + minutes * 60 + seconds) * 1000 + milliseconds;

            QString chapterTitle = line.mid(match.capturedLength()).trimmed();
            chapters.append(std::make_pair(totalMilliseconds, chapterTitle));
        }
    }

    file.close();
    return chapters;
}

QList<std::pair<qint64, QString> > PlaybackProgressIndicator::parsePBFChapterFile(const QString &filePath)
{
    QList<std::pair<qint64, QString>> chapters;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return chapters;
    }

    QTextStream in(&file);
    QRegularExpression chapterRegex(R"(^\d+=(\d+)\*([^*]*)\*.*$)");

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QRegularExpressionMatch match = chapterRegex.match(line);
        if (match.hasMatch()) {
            qint64 timestamp = match.captured(1).toLongLong();
            QString title = match.captured(2).trimmed();
            chapters.append(std::make_pair(timestamp, title));
        }
    }

    file.close();
    return chapters;
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
            if (chapterStartTime == 0) continue;
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
