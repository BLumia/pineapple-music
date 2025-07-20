// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>
#include <QMouseEvent>
#include <QStandardItemModel>

class PlaybackProgressIndicator : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool seekOnMove MEMBER m_seekOnMove NOTIFY seekOnMoveChanged)
    Q_PROPERTY(qint64 position MEMBER m_position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration MEMBER m_duration NOTIFY durationChanged)
public:
    enum Roles {
        ChapterTitleRole = Qt::DisplayRole,
        StartTimeMsRole = Qt::UserRole + 1,
    };

    explicit PlaybackProgressIndicator(QWidget *parent = nullptr);
    ~PlaybackProgressIndicator() = default;

    QStandardItemModel* chapterModel() { return &m_chapterModel; }
    QModelIndex currentChapterItem() const;
    QString currentChapterName() const;

    void setPosition(qint64 pos);
    void setDuration(qint64 dur);
    void setChapters(QList<std::pair<qint64, QString>> chapters);

    static QString formatTime(qint64 milliseconds);
    static QList<std::pair<qint64, QString>> tryLoadChapters(const QString & filePath);
    static QList<std::pair<qint64, QString>> tryLoadSidecarChapterFile(const QString & filePath);
    static QList<std::pair<qint64, QString>> tryLoadChaptersFromMetadata(const QString & filePath);
    static QList<std::pair<qint64, QString>> parseCHPChapterFile(const QString & filePath);
    static QList<std::pair<qint64, QString>> parsePBFChapterFile(const QString & filePath);

signals:
    void seekOnMoveChanged(bool sow);
    void positionChanged(qint64 newPosition);
    void durationChanged(qint64 newDuration);
    void seekingRequested(qint64 position);

public slots:

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_seekOnMove = true;
    qint64 m_position = -1;
    qint64 m_seekingPosition = -1;
    qint64 m_duration = -1;
    QStandardItemModel m_chapterModel;
};

