#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "playlistmanager.h"

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
#include <QWindow>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_playlistManager(new PlaylistManager(this))
{
    ui->setupUi(this);

    m_playlistManager->setAutoLoadFilterSuffixes({
        "*.mp3", "*.wav", "*.aiff", "*.ape", "*.flac", "*.ogg", "*.oga", "*.mpga"
    });
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_mediaPlayer->setLoops(QMediaPlayer::Infinite);
    ui->playlistView->setModel(m_playlistManager->model());

    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
    this->setAttribute(Qt::WA_TranslucentBackground, true);

    initConnections();
    initUiAndAnimation();

    centerWindow();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::commandlinePlayAudioFiles(QStringList audioFiles)
{
    QList<QUrl> audioFileUrls = strlst2urllst(audioFiles);

    if (!audioFileUrls.isEmpty()) {
        QModelIndex modelIndex = m_playlistManager->loadPlaylist(audioFileUrls);
        if (modelIndex.isValid()) {
            m_mediaPlayer->setSource(m_playlistManager->urlByIndex(modelIndex));
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

    if (sampleRate >= 0) {
        uiStrs << QString("%1 Hz").arg(sampleRate);
        tooltipStrs << tr("Sample Rate: %1 Hz").arg(sampleRate);
    }

    if (bitrate >= 0) {
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

    // Temp bg
    painter.setBrush(QColor(20, 32, 83));
    painter.drawRect(0, 0, width(), height());

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

    QModelIndex modelIndex = m_playlistManager->loadPlaylist(urls);
    if (modelIndex.isValid()) {
        m_mediaPlayer->setSource(m_playlistManager->urlByIndex(modelIndex));
        play();
    }
}

void MainWindow::loadFile()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
                                                      tr("Select songs to play"),
                                                      QDir::homePath(),
                                                      tr("Audio Files") + " (*.mp3 *.wav *.aiff *.ape *.flac *.ogg *.oga)");
    QList<QUrl> urlList;
    for (const QString & fileName : files) {
        urlList.append(QUrl::fromLocalFile(fileName));
    }

    m_playlistManager->loadPlaylist(urlList);
    m_mediaPlayer->setSource(urlList.first());
}

void MainWindow::play()
{
    QUrl fileUrl(m_mediaPlayer->source());

    m_mediaPlayer->play();

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
        }
#endif // NO_TAGLIB
    }
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

QString MainWindow::ms2str(qint64 ms)
{
    QTime duaTime(QTime::fromMSecsSinceStartOfDay(ms));
    if (duaTime.hour() > 0) {
        return duaTime.toString("h:mm:ss");
    } else {
        return duaTime.toString("m:ss");
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

void MainWindow::on_playbackSlider_valueChanged(int value)
{
    qint64 currPos = m_mediaPlayer->duration() == 0 ? value : m_mediaPlayer->position() * ui->playbackSlider->maximum() / m_mediaPlayer->duration();
    if (qAbs(currPos - value) > 2) {
        m_mediaPlayer->setPosition(ui->playbackSlider->value() * 1.0 / ui->playbackSlider->maximum() * m_mediaPlayer->duration());
    }
}

void MainWindow::on_prevBtn_clicked()
{
    QModelIndex index(m_playlistManager->previousIndex());
    m_playlistManager->setCurrentIndex(index);
    m_mediaPlayer->setSource(m_playlistManager->urlByIndex(index));
    play();
}

void MainWindow::on_nextBtn_clicked()
{
    QModelIndex index(m_playlistManager->nextIndex());
    m_playlistManager->setCurrentIndex(index);
    m_mediaPlayer->setSource(m_playlistManager->urlByIndex(index));
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
    m_bgLinearGradient.setColorAt(0, QColor(255, 255, 255, 25)); // a:0
    m_bgLinearGradient.setColorAt(1, QColor(255, 255, 255, 75)); // a:200
    m_bgLinearGradient.setStart(0, 0);
    m_bgLinearGradient.setFinalStop(0, height());

    m_fadeOutAnimation = new QPropertyAnimation(this, "windowOpacity", this);
    m_fadeOutAnimation->setDuration(400);
    m_fadeOutAnimation->setStartValue(1);
    m_fadeOutAnimation->setEndValue(0);
    connect(m_fadeOutAnimation, &QPropertyAnimation::finished, this, &QMainWindow::close);
    setFixedSize(490, 160);
}

void MainWindow::initConnections()
{
    connect(m_mediaPlayer, &QMediaPlayer::metaDataChanged, this, [=](){
        QMediaMetaData metadata(m_mediaPlayer->metaData());
        setAudioMetadataForDisplay(metadata.stringValue(QMediaMetaData::Title),
                                   metadata.stringValue(QMediaMetaData::Author),
                                   metadata.stringValue(QMediaMetaData::AlbumTitle));
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
        ui->nowTimeLabel->setText(ms2str(pos));
        if (m_mediaPlayer->duration() != 0) {
            ui->playbackSlider->setSliderPosition(ui->playbackSlider->maximum() * pos / m_mediaPlayer->duration());
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
        ui->totalTimeLabel->setText(ms2str(dua));
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

   connect(m_mediaPlayer, &QMediaPlayer::errorOccurred, this, [=](QMediaPlayer::Error error, const QString &errorString) {
        qDebug() << error << errorString;
   });
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

void MainWindow::on_playListBtn_clicked()
{
    setFixedHeight(size().height() < 200 ? 420 : 160);
}

void MainWindow::on_playlistView_activated(const QModelIndex &index)
{
    m_playlistManager->setCurrentIndex(index);
    m_mediaPlayer->setSource(m_playlistManager->urlByIndex(index));
    play();
}

