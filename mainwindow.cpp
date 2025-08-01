// SPDX-FileCopyrightText: 2025 Gary Wang <opensource@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "playlistmanager.h"
#include "fftspectrum.h"
#include "lrcbar.h"
#include "taskbarmanager.h"

// taglib
#ifndef NO_TAGLIB
#include <fileref.h>
#endif // NO_TAGLIB

#include <QPainter>
#include <QMediaPlayer>
#include <QMediaMetaData>
#include <QAudioOutput>
#include <QPropertyAnimation>
#include <QFileDialog>
#include <QTime>
#include <QStyle>
#include <QScreen>
#include <QListView>
#include <QCollator>
#include <QMimeData>
#include <QMenu>
#include <QWindow>
#include <QStandardPaths>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QMessageBox>
#include <QStringBuilder>
#include <QSettings>
#include <QGraphicsDropShadowEffect>
#include <QTimer>

constexpr QSize miniSize(490, 160);
constexpr QSize fullSize(490, 420);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_mediaDevices(new QMediaDevices(this))
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_fftSpectrum(new FFTSpectrum(this))
    , m_lrcbar(new LrcBar(nullptr))
    , m_playlistManager(new PlaylistManager(this))
    , m_taskbarManager(new TaskBarManager(this))
{
    ui->setupUi(this);
    m_playlistManager->setAutoLoadFilterSuffixes({
        "*.mp3", "*.wav", "*.aiff", "*.ape", "*.flac", "*.ogg", "*.oga", "*.mpga", "*.aac", "*.tta"
    });
    m_fftSpectrum->setMediaPlayer(m_mediaPlayer);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_mediaPlayer->setLoops(QMediaPlayer::Infinite);
    ui->playlistView->setModel(m_playlistManager->model());

    ui->chapterNameBtn->setVisible(false);
    ui->chapterlistView->setModel(ui->playbackProgressIndicator->chapterModel());
    ui->chapterlistView->setRootIsDecorated(false);

    ui->actionHelp->setShortcut(QKeySequence::HelpContents);
    addAction(ui->actionHelp);
    ui->actionOpen->setShortcut(QKeySequence::Open);
    addAction(ui->actionOpen);

    ui->titleLabel->setGraphicsEffect(createLabelShadowEffect());
    ui->propLabel->setGraphicsEffect(createLabelShadowEffect());
    ui->nowTimeLabel->setGraphicsEffect(createLabelShadowEffect());
    ui->totalTimeLabel->setGraphicsEffect(createLabelShadowEffect());

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_taskbarManager->setCanTogglePlayback(true);
    m_taskbarManager->setCanSkipBackward(true);
    m_taskbarManager->setCanSkipForward(true);
    m_taskbarManager->setShowProgress(true);

    loadConfig();
    loadSkinData();
    initConnections();
    initUiAndAnimation();

    centerWindow();

    QTimer::singleShot(1000, [this](){
        m_taskbarManager->setWinId(window()->winId());
    });
}

MainWindow::~MainWindow()
{
    saveConfig();
    delete m_lrcbar;
    delete ui;
}

void MainWindow::commandlinePlayAudioFiles(QStringList audioFiles)
{
    QList<QUrl> audioFileUrls = strlst2urllst(audioFiles);

    if (!audioFileUrls.isEmpty()) {
        QModelIndex modelIndex = m_playlistManager->loadPlaylist(audioFileUrls);
        if (modelIndex.isValid()) {
            loadByModelIndex(modelIndex);
            play();
        }
    }
}

void MainWindow::setAudioPropertyInfoForDisplay(int sampleRate, int bitrate, int channelCount, QString audioExt)
{
    QStringList uiStrs;
    QStringList tooltipStrs;

    auto channelStr = [](int channelCnt) {
        switch (channelCnt) {
        case 1:
            return tr("Mono");
        case 2:
            return tr("Stereo");
        default:
            return tr("%1 Channels").arg(channelCnt);
        }
    };

    if (sampleRate > 0) {
        uiStrs << QString("%1 Hz").arg(sampleRate);
        tooltipStrs << tr("Sample Rate: %1 Hz").arg(sampleRate);
    }

    if (bitrate > 0) {
        uiStrs << QString("%1 Kbps").arg(bitrate);
        tooltipStrs << tr("Bitrate: %1 Kbps").arg(bitrate);
    }

    if (channelCount >= 0) {
        uiStrs << channelStr(channelCount);
        tooltipStrs << tr("Channel Count: %1").arg(channelCount);
    }

    uiStrs << audioExt;

    ui->propLabel->setText(uiStrs.join(" | "));
    ui->propLabel->setToolTip(tooltipStrs.join('\n'));
}

void MainWindow::setAudioMetadataForDisplay(QString title, QString artist, QString album)
{
    Q_UNUSED(album);

    if (!title.isEmpty()) {
        if (!artist.isEmpty()) {
            ui->titleLabel->setText(QString("%1 - %2").arg(artist, title));
        } else if (!album.isEmpty()) {
            ui->titleLabel->setText(QString("%1 - %2").arg(album, title));
        } else {
            ui->titleLabel->setText(QString("%1").arg(title));
        }
    }
}

void MainWindow::localSocketPlayAudioFiles(QVariant audioFilesVariant)
{
    QStringList urlStrList = audioFilesVariant.toStringList();
    qDebug() << urlStrList << "MainWindow::localSocketPlayAudioFiles";
    commandlinePlayAudioFiles(urlStrList);

    showNormal();
    activateWindow();
    raise();
}

void MainWindow::closeEvent(QCloseEvent *)
{
    qApp->exit();
}

void MainWindow::paintEvent(QPaintEvent * e)
{
    QPainter painter(this);

    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_skin.isNull()) {
        painter.setBrush(QColor(40, 50, 123));
        painter.drawRect(0, 0, width(), height());
    } else {
        painter.drawPixmap(0, 0, m_skin);
    }

    painter.setBrush(QBrush(m_bgLinearGradient));
    painter.drawRect(0, 0, width(), height());

    return QMainWindow::paintEvent(e);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && !isMaximized()) {
        m_clickedOnWindow = true;
        event->accept();
    }

    return QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && m_clickedOnWindow) {
        window()->windowHandle()->startSystemMove();
        event->accept();
    }

    return QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_clickedOnWindow = false;
    return QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    // TODO: file/format filter?

    e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *e)
{
    QList<QUrl> urls = e->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }

    QString fileName = urls.first().toLocalFile();
    if (fileName.isEmpty()) {
        return;
    }

    if (fileName.endsWith(".png") || fileName.endsWith(".jpg") ||
        fileName.endsWith(".jpeg") || fileName.endsWith(".gif")) {
        setSkin(fileName, true);
        return;
    }

    if (fileName.endsWith(".lrc")) {
        m_lrcbar->loadLyrics(fileName);
        return;
    }

    if (fileName.endsWith(".chp") || fileName.endsWith(".pbf")) {
        QList<std::pair<qint64, QString>> chapters(PlaybackProgressIndicator::tryLoadSidecarChapterFile(fileName));
        ui->playbackProgressIndicator->setChapters(chapters);
        return;
    }

    if (fileName.endsWith(".m3u") || fileName.endsWith(".m3u8")) {
        const QModelIndex & modelIndex = m_playlistManager->loadM3U8Playlist(urls.constFirst());
        if (modelIndex.isValid()) {
            loadByModelIndex(modelIndex);
            play();
        }
        return;
    }

    const QModelIndex & modelIndex = m_playlistManager->loadPlaylist(urls);
    if (modelIndex.isValid()) {
        loadByModelIndex(modelIndex);
        play();
    }
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu * menu = new QMenu;
    menu->addAction(ui->actionHelp);
    menu->exec(mapToGlobal(event->pos()));
    menu->deleteLater();

    return QMainWindow::contextMenuEvent(event);
}

void MainWindow::loadFile()
{
    QStringList musicFolders(QStandardPaths::standardLocations(QStandardPaths::MusicLocation));
    musicFolders.append(QDir::homePath());
    QStringList files = QFileDialog::getOpenFileNames(this,
                                                      tr("Select songs to play"),
                                                      musicFolders.first(),
                                                      tr("Audio Files") + " (*.mp3 *.wav *.aiff *.ape *.flac *.ogg *.oga)");
    if (files.isEmpty()) return;
    QList<QUrl> urlList;
    for (const QString & fileName : files) {
        urlList.append(QUrl::fromLocalFile(fileName));
    }

    const QModelIndex & modelIndex = m_playlistManager->loadPlaylist(urlList);
    loadByModelIndex(modelIndex);
}

void MainWindow::loadFile(const QUrl &url)
{
    const QString filePath = url.toLocalFile();
    m_mediaPlayer->setSource(url);
    m_lrcbar->loadLyrics(filePath);
    QList<std::pair<qint64, QString>> chapters(PlaybackProgressIndicator::tryLoadChapters(filePath));
    ui->playbackProgressIndicator->setChapters(chapters);
}

void MainWindow::loadByModelIndex(const QModelIndex & index)
{
    loadFile(m_playlistManager->urlByIndex(index));
}

void MainWindow::play()
{
    m_mediaPlayer->play();
}

void MainWindow::setSkin(QString imagePath, bool save)
{
    m_skin = QPixmap(imagePath);
    if (save) {
        saveSkinData();
    }
    m_skin = m_skin.scaled(fullSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    update();
}

void MainWindow::centerWindow()
{
    this->setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            this->size(),
            qApp->screenAt(QCursor::pos())->geometry()
        )
    );
}

void MainWindow::on_closeWindowBtn_clicked()
{
    m_fadeOutAnimation->start();
}

void MainWindow::on_playBtn_clicked()
{
    if (m_mediaPlayer->mediaStatus() == QMediaPlayer::NoMedia) {
        loadFile();
        play();
    } else if (m_mediaPlayer->mediaStatus() == QMediaPlayer::InvalidMedia) {
        ui->propLabel->setText("Error: InvalidMedia" + m_mediaPlayer->errorString());
    } else {
        if (QList<QMediaPlayer::PlaybackState> {QMediaPlayer::PausedState, QMediaPlayer::StoppedState}
                .contains(m_mediaPlayer->playbackState())) {
            m_mediaPlayer->play();
        } else {
            m_mediaPlayer->pause();
        }
    }
}

QList<QUrl> MainWindow::strlst2urllst(QStringList strlst)
{
    QList<QUrl> urlList;
    for (const QString & str : strlst) {
        QUrl url = QUrl::fromLocalFile(str);
        if (url.isValid()) {
            urlList.append(url);
        }
    }

    return urlList;
}

void MainWindow::on_volumeSlider_valueChanged(int value)
{
    if (m_audioOutput->isMuted()) {
        m_audioOutput->setMuted(false);
    }
    m_audioOutput->setVolume(value / 100.0);
}

void MainWindow::on_stopBtn_clicked()
{
    m_mediaPlayer->stop();
}

void MainWindow::on_prevBtn_clicked()
{
    QModelIndex index(m_playlistManager->previousIndex());
    m_playlistManager->setCurrentIndex(index);
    loadByModelIndex(index);
    play();
}

void MainWindow::on_nextBtn_clicked()
{
    QModelIndex index(m_playlistManager->nextIndex());
    m_playlistManager->setCurrentIndex(index);
    loadByModelIndex(index);
    play();
}

void MainWindow::on_volumeBtn_clicked()
{
    m_audioOutput->setMuted(!m_audioOutput->isMuted());
}

void MainWindow::on_minimumWindowBtn_clicked()
{
    this->showMinimized();
}

void MainWindow::initUiAndAnimation()
{
    m_bgLinearGradient.setColorAt(0, QColor(0, 0, 0, 25));
    m_bgLinearGradient.setColorAt(1, QColor(0, 0, 0, 80));
    m_bgLinearGradient.setStart(0, 0);
    m_bgLinearGradient.setFinalStop(0, height());

    m_fadeOutAnimation = new QPropertyAnimation(this, "windowOpacity", this);
    m_fadeOutAnimation->setDuration(400);
    m_fadeOutAnimation->setStartValue(1);
    m_fadeOutAnimation->setEndValue(0);
    connect(m_fadeOutAnimation, &QPropertyAnimation::finished, this, &QMainWindow::close);
    setFixedSize(miniSize);
}

void MainWindow::initConnections()
{
    connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, [=]{
        m_audioOutput->setDevice(m_mediaDevices->defaultAudioOutput());
    });

    connect(ui->playbackProgressIndicator, &PlaybackProgressIndicator::seekingRequested, this, [=](qint64 pos){
        m_mediaPlayer->setPosition(pos);
    });

    connect(m_mediaPlayer, &QMediaPlayer::sourceChanged, this, [=](){
        QUrl fileUrl(m_mediaPlayer->source());

        ui->titleLabel->setText(fileUrl.fileName());
        ui->titleLabel->setToolTip(fileUrl.fileName());

        if (fileUrl.isLocalFile()) {
            QString filePath(fileUrl.toLocalFile());
            QString suffix(filePath.mid(filePath.lastIndexOf('.') + 1));
            suffix = suffix.toUpper();

#ifndef NO_TAGLIB
            TagLib::FileRef fileRef(filePath.toLocal8Bit().data());

            if (!fileRef.isNull() && fileRef.audioProperties()) {
                TagLib::AudioProperties *prop = fileRef.audioProperties();
                setAudioPropertyInfoForDisplay(prop->sampleRate(), prop->bitrate(), prop->channels(), suffix);
            } else {
                qDebug() << "No Audio Properties from TagLib";
            }

            if (!fileRef.isNull() && fileRef.tag()) {
                TagLib::Tag * tag = fileRef.tag();
                setAudioMetadataForDisplay(QString::fromStdString(tag->title().to8Bit(true)),
                                           QString::fromStdString(tag->artist().to8Bit(true)),
                                           QString::fromStdString(tag->album().to8Bit(true)));
                m_urlMissingTagLibMetadata.clear();
            } else {
                qDebug() << "No Audio Metadata from TagLib";
                m_urlMissingTagLibMetadata = fileUrl;
            }
#endif // NO_TAGLIB
        }
    });

    connect(m_mediaPlayer, &QMediaPlayer::metaDataChanged, this, [=](){
        QMediaMetaData metadata(m_mediaPlayer->metaData());
        // it's known in some cases QMediaMetaData using the incorrect text codec for metadata
        // see `02  Yoiyami Hanabi.mp3`'s Title. So we don't use Qt's one if tablib is available.
        qDebug() << metadata.stringValue(QMediaMetaData::Title) << metadata.stringValue(QMediaMetaData::Author);
#ifdef NO_TAGLIB
        bool needMetadataFromQt = true;
#else
        bool needMetadataFromQt = m_urlMissingTagLibMetadata == m_mediaPlayer->source();
#endif // NO_TAGLIB
        if (needMetadataFromQt) {
            setAudioMetadataForDisplay(metadata.stringValue(QMediaMetaData::Title),
                                       metadata.stringValue(QMediaMetaData::Author),
                                       metadata.stringValue(QMediaMetaData::AlbumTitle));
            setAudioPropertyInfoForDisplay(-1, metadata.value(QMediaMetaData::AudioBitRate).toInt() / 1000,
                                           -1, metadata.stringValue(QMediaMetaData::FileFormat));
        }
        QVariant coverArt(metadata.value(QMediaMetaData::ThumbnailImage));
        if (!coverArt.isNull()) {
            ui->coverLabel->setPixmap(QPixmap::fromImage(coverArt.value<QImage>()));
        } else {
            qDebug() << "No ThumbnailImage!" << metadata.keys();
            ui->coverLabel->setPixmap(QPixmap(":/icons/icons/media-album-cover.svg"));
        }
    });
    connect(m_playlistManager, &PlaylistManager::currentIndexChanged, this, [=](int index){
        ui->playlistView->setCurrentIndex(m_playlistManager->model()->index(index));
    });

    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, [=](qint64 pos) {
        ui->nowTimeLabel->setText(PlaybackProgressIndicator::formatTime(pos));
        if (m_mediaPlayer->duration() != 0) {
            ui->playbackProgressIndicator->setPosition(pos);
            m_taskbarManager->setProgressValue(pos);
        }
        m_lrcbar->playbackPositionChanged(pos, m_mediaPlayer->duration());

        static QString lastChapterName;
        if (ui->playbackProgressIndicator->chapterModel()->rowCount() > 0) {
            QString currentChapterName = ui->playbackProgressIndicator->currentChapterName();
            if (currentChapterName != lastChapterName) {
                ui->chapterNameBtn->setText(currentChapterName);
                lastChapterName = currentChapterName;
            }
            ui->chapterNameBtn->setVisible(true);
        } else {
            if (!lastChapterName.isEmpty()) {
                ui->chapterNameBtn->setText("");
                lastChapterName.clear();
            }
            ui->chapterNameBtn->setVisible(false);
        }
    });

    connect(m_audioOutput, &QAudioOutput::mutedChanged, this, [=](bool muted) {
        if (muted) {
            ui->volumeBtn->setIcon(QIcon(":/icons/icons/audio-volume-muted.png"));
        } else {
            ui->volumeBtn->setIcon(QIcon(":/icons/icons/audio-volume-high.png"));
        }
    });

    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, [=](qint64 dua) {
        ui->playbackProgressIndicator->setDuration(dua);
        m_taskbarManager->setProgressMaximum(dua);
        ui->totalTimeLabel->setText(PlaybackProgressIndicator::formatTime(dua));
    });

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [=](QMediaPlayer::PlaybackState newState) {
        switch (newState) {
        case QMediaPlayer::PlayingState:
            ui->playBtn->setIcon(QIcon(":/icons/icons/media-playback-pause.png"));
            break;
        case QMediaPlayer::StoppedState:
        case QMediaPlayer::PausedState:
            ui->playBtn->setIcon(QIcon(":/icons/icons/media-playback-start.png"));
            break;
        }
        m_taskbarManager->setPlaybackState(newState);
    });

    connect(m_audioOutput, &QAudioOutput::volumeChanged, this, [=](float vol) {
        ui->volumeSlider->setValue(vol * 100);
    });

    connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::EndOfMedia) {
            switch (m_playbackMode) {
            case MainWindow::CurrentItemOnce:
                // do nothing
                break;
            case MainWindow::CurrentItemInLoop:
                // also do nothing
                // as long as we did `setLoops(Infinite)`, we won't even get there
                break;
            case MainWindow::Sequential:
                on_nextBtn_clicked();
                break;
            }
        }
    });

    connect(this, &MainWindow::playbackModeChanged, this, [=](){
        switch (m_playbackMode) {
        case MainWindow::CurrentItemOnce:
            m_mediaPlayer->setLoops(QMediaPlayer::Once);
            ui->playbackModeBtn->setIcon(QIcon(":/icons/icons/media-repeat-single.png"));
            break;
        case MainWindow::CurrentItemInLoop:
            m_mediaPlayer->setLoops(QMediaPlayer::Infinite);
            ui->playbackModeBtn->setIcon(QIcon(":/icons/icons/media-playlist-repeat-song.png"));
            break;
        case MainWindow::Sequential:
            m_mediaPlayer->setLoops(QMediaPlayer::Once);
            ui->playbackModeBtn->setIcon(QIcon(":/icons/icons/media-playlist-repeat.png"));
            break;
        }
    });

    connect(m_taskbarManager, &TaskBarManager::togglePlayback, this, [this](){
        on_playBtn_clicked();
    });

    connect(m_taskbarManager, &TaskBarManager::skipBackward, this, [this](){
        on_prevBtn_clicked();
    });

    connect(m_taskbarManager, &TaskBarManager::skipForward, this, [this](){
        on_nextBtn_clicked();
    });

    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred, this, [=](QMediaPlayer::Error error, const QString &errorString) {
        qDebug() << error << errorString;
    });
}

void MainWindow::loadConfig()
{
    QDir configDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    QSettings settings(configDir.filePath("settings.ini"), QSettings::IniFormat);
    ui->volumeSlider->setValue(settings.value("volume", 100).toInt());
}

void MainWindow::saveConfig()
{
    QDir configDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }
    QSettings settings(configDir.filePath("settings.ini"), QSettings::IniFormat);
    settings.setValue("volume", ui->volumeSlider->value());
}

void MainWindow::loadSkinData()
{
    QDir configDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    QFile file(configDir.filePath("skin.dat"));
    bool canOpen = file.open(QIODevice::ReadOnly);
    if (!canOpen) return;
    QDataStream stream(&file);
    quint32 magic;
    stream >> magic;
    if (magic == 0x78297000) {
        stream >> m_skin;
        m_skin = m_skin.scaled(fullSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }
    file.close();
}

void MainWindow::saveSkinData()
{
    QDir configDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!configDir.exists()) {
        configDir.mkpath(".");
    }
    QFile file(configDir.absoluteFilePath("skin.dat"));
    file.open(QIODevice::WriteOnly);
    QDataStream stream(&file);
    stream << (quint32)0x78297000 << m_skin;
    file.close();
}

void MainWindow::on_playbackModeBtn_clicked()
{
    switch (m_playbackMode) {
    case MainWindow::CurrentItemOnce:
        setProperty("playbackMode", MainWindow::CurrentItemInLoop);
        break;
    case MainWindow::CurrentItemInLoop:
        setProperty("playbackMode", MainWindow::Sequential);
        break;
    case MainWindow::Sequential:
        setProperty("playbackMode", MainWindow::CurrentItemOnce);
        break;
    default:
        break;
    }
}

void MainWindow::on_setSkinBtn_clicked()
{
    QStringList imageFolders(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation));
    imageFolders.append(QDir::homePath());
    QString image = QFileDialog::getOpenFileName(this, tr("Select image as background skin"),
                                                 imageFolders.first(),
                                                 tr("Image files (*.jpg *.jpeg *.png *.gif)"));
    if(!image.isEmpty()) {
        setSkin(image, true);
    }
}

void MainWindow::on_playListBtn_clicked()
{
    if (size().height() < 200) {
        setFixedSize(fullSize);
        ui->pluginStackedWidget->setCurrentWidget(ui->playlistViewPage);
    } else {
        if (ui->pluginStackedWidget->currentWidget() == ui->playlistViewPage) {
            setFixedSize(miniSize);
        } else {
            ui->pluginStackedWidget->setCurrentWidget(ui->playlistViewPage);
        }
    }
}

void MainWindow::on_playlistView_activated(const QModelIndex &index)
{
    m_playlistManager->setCurrentIndex(index);
    loadByModelIndex(index);
    play();
}

void MainWindow::on_lrcBtn_clicked()
{
    if (m_lrcbar->isVisible()) {
        m_lrcbar->hide();
    } else {
        m_lrcbar->show();
    }
}

void MainWindow::on_chapterlistView_activated(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QModelIndex timeColumnIndex = index.sibling(index.row(), 0);
    QStandardItem* timeItem = ui->playbackProgressIndicator->chapterModel()->itemFromIndex(timeColumnIndex);
    if (!timeItem) return;

    qint64 chapterStartTime = timeItem->data(PlaybackProgressIndicator::StartTimeMsRole).toLongLong();
    m_mediaPlayer->setPosition(chapterStartTime);
}

void MainWindow::on_chapterNameBtn_clicked()
{
    if (size().height() < 200) {
        setFixedSize(fullSize);
    }
    ui->pluginStackedWidget->setCurrentWidget(ui->chaptersViewPage);
    if (ui->playbackProgressIndicator->chapterModel()->rowCount() > 0) {
        const QModelIndex & curChapterItem = ui->playbackProgressIndicator->currentChapterItem();
        if (curChapterItem.isValid()) {
            ui->chapterlistView->setCurrentIndex(curChapterItem);
            ui->chapterlistView->scrollTo(curChapterItem, QAbstractItemView::EnsureVisible);
        }
    }
}

void MainWindow::on_actionOpen_triggered()
{
    loadFile();
}

void MainWindow::on_actionHelp_triggered()
{
    QMessageBox infoBox(this);
    infoBox.setIcon(QMessageBox::Information);
    infoBox.setWindowTitle(tr("About"));
    infoBox.setStandardButtons(QMessageBox::Ok);
    infoBox.setText(
        tr("Pineapple Music") %
        "\n\n" %
        tr("Based on the following free software libraries:") %
        "\n\n" %
        QStringLiteral("- [Qt](https://www.qt.io/) %1 with the following module(s):\n").arg(QT_VERSION_STR) %
        QStringLiteral("  - multimedia\n") %
#ifdef USE_QTEXTCODEC
        QStringLiteral("  - core5compat\n") %
#endif
#ifndef NO_TAGLIB
        QStringLiteral("- [TagLib](https://github.com/taglib/taglib)\n") %
#endif // NO_TAGLIB
#ifdef HAVE_KCODECS
        QStringLiteral("- [KCodecs](https://invent.kde.org/frameworks/kcodecs)\n") %
#endif // NO_TAGLIB
#ifdef HAVE_FFMPEG
        QStringLiteral("- [FFmpeg](https://ffmpeg.org/)\n") %
#endif // HAVE_FFMPEG
        "\n"
        "[Source Code](https://github.com/BLumia/pineapple-music)\n"
        "\n"
        "Copyright &copy; 2025 [BLumia](https://github.com/BLumia/)"
        );
    infoBox.setTextFormat(Qt::MarkdownText);
    infoBox.exec();
}

QGraphicsDropShadowEffect *MainWindow::createLabelShadowEffect()
{
    QGraphicsDropShadowEffect * effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(3);
    effect->setColor(QColor(0, 0, 0, 180));
    effect->setOffset(1, 1);
    return effect;
}
