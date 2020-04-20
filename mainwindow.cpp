#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "playlistmodel.h"

#include "ID3v2Pic.h"
#include "FlacPic.h"

// taglib
#include <fileref.h>

#include <QPainter>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QPropertyAnimation>
#include <QFileDialog>
#include <QTime>
#include <QStyle>
#include <QScreen>
#include <QListView>
#include <QCollator>
#include <QMimeData>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_playlistModel(new PlaylistModel(this))
{
    ui->setupUi(this);

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
        if (audioFileUrls.count() == 1) {
            loadPlaylistBySingleLocalFile(audioFileUrls.first().toLocalFile());
        } else {
            createPlaylist(audioFileUrls);
        }
        m_mediaPlayer->play();
    }
}

void MainWindow::loadPlaylistBySingleLocalFile(const QString &path)
{
    QFileInfo info(path);
    QDir dir(info.path());
    QString currentFileName = info.fileName();
    QStringList entryList = dir.entryList({"*.mp3", "*.wav", "*.aiff", "*.ape", "*.flac", "*.ogg", "*.oga"},
                                          QDir::Files | QDir::NoSymLinks, QDir::NoSort);

    QCollator collator;
    collator.setNumericMode(true);

    std::sort(entryList.begin(), entryList.end(), collator);

    QList<QUrl> urlList;
    int currentFileIndex = -1;
    for (int i = 0; i < entryList.count(); i++) {
        const QString & oneEntry = entryList.at(i);
        urlList.append(QUrl::fromLocalFile(dir.absoluteFilePath(oneEntry)));
        if (oneEntry == currentFileName) {
            currentFileIndex = i;
        }
    }

    QMediaPlaylist * playlist = createPlaylist(urlList);
    playlist->setCurrentIndex(currentFileIndex);
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
        tooltipStrs << QString("Sample Rate: %1 Hz").arg(sampleRate);
    }

    if (bitrate >= 0) {
        uiStrs << QString("%1 Kbps").arg(bitrate);
        tooltipStrs << QString("Bitrate: %1 Kbps").arg(bitrate);
    }

    if (channelCount >= 0) {
        uiStrs << channelStr(channelCount);
        tooltipStrs << QString("Channel Count: %1").arg(channelCount);
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
            ui->titleLabel->setText(QString("%1 - %2").arg(artist, album));
        } else {
            ui->titleLabel->setText(QString("%1").arg(artist));
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
        m_oldMousePos = event->pos();
        event->accept();
    }

    return QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton && m_clickedOnWindow) {
        move(event->globalPos() - m_oldMousePos);
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

    // TODO: file/format filter?

    createPlaylist(urls);
    m_mediaPlayer->play();
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

    createPlaylist(urlList);
}

/*
 * The returned QMediaPlaylist* ownership belongs to the internal QMediaPlayer instance.
 */
QMediaPlaylist *MainWindow::createPlaylist(QList<QUrl> urlList)
{
    QMediaPlaylist * oldPlaylist = m_mediaPlayer->playlist();
    QMediaPlaylist * playlist = new QMediaPlaylist(m_mediaPlayer);

    if (oldPlaylist) {
        oldPlaylist->disconnect();
        oldPlaylist->deleteLater();
    }

    for (const QUrl & url : urlList) {
        bool succ = playlist->addMedia(QMediaContent(url));
        if (!succ) {
            qDebug("!!!!!!!!! break point time !!!!!!!!!");
        }
    }

    connect(playlist, &QMediaPlaylist::playbackModeChanged, this, [=](QMediaPlaylist::PlaybackMode mode) {
        switch (mode) {
        case QMediaPlaylist::CurrentItemInLoop:
            ui->playbackModeBtn->setIcon(QIcon(":/icons/icons/media-playlist-repeat-song.png"));
            break;
        case QMediaPlaylist::Loop:
            ui->playbackModeBtn->setIcon(QIcon(":/icons/icons/media-playlist-repeat.png"));
            break;
        case QMediaPlaylist::Sequential:
            ui->playbackModeBtn->setIcon(QIcon(":/icons/icons/media-playlist-normal.png"));
            break;
        case QMediaPlaylist::Random:
            ui->playbackModeBtn->setIcon(QIcon(":/icons/icons/media-playlist-shuffle.png"));
            break;
        default:
            break;
        }
    });

    playlist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
    m_mediaPlayer->setPlaylist(playlist);
    m_playlistModel->setPlaylist(playlist);

    return playlist;
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
        m_mediaPlayer->play();
    } else if (m_mediaPlayer->mediaStatus() == QMediaPlayer::InvalidMedia) {
        ui->propLabel->setText("Error: InvalidMedia");
    } else {
        if (QList<QMediaPlayer::State> {QMediaPlayer::PausedState, QMediaPlayer::StoppedState}
                .contains(m_mediaPlayer->state())) {
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
    if (m_mediaPlayer->isMuted()) {
        m_mediaPlayer->setMuted(false);
    }
    m_mediaPlayer->setVolume(value);
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
    // QMediaPlaylist::previous() won't work when in CurrentItemInLoop playmode,
    // and also works not as intended when in other playmode, so do it manually...
    QMediaPlaylist * playlist = m_mediaPlayer->playlist();
    if (playlist) {
        int index = playlist->currentIndex();
        int count = playlist->mediaCount();

        m_mediaPlayer->playlist()->setCurrentIndex(index == 0 ? count - 1 : index - 1);
    }
}

void MainWindow::on_nextBtn_clicked()
{
    // see also: MainWindow::on_prevBtn_clicked()
    QMediaPlaylist * playlist = m_mediaPlayer->playlist();
    if (playlist) {
        int index = playlist->currentIndex();
        int count = playlist->mediaCount();

        m_mediaPlayer->playlist()->setCurrentIndex(index == (count - 1) ? 0 : index + 1);
    }
}

void MainWindow::on_volumeBtn_clicked()
{
    m_mediaPlayer->setMuted(!m_mediaPlayer->isMuted());
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

    // temp: a playlist for debug...
    QListView * tmp_listview = new QListView(ui->pluginWidget);
    tmp_listview->setModel(m_playlistModel);
    tmp_listview->setGeometry({0,0,490,250});
    this->setGeometry({0,0,490,160}); // temp size, hide the playlist thing.
}

void MainWindow::initConnections()
{
    connect(m_mediaPlayer, &QMediaPlayer::currentMediaChanged, this, [=](const QMediaContent &media) {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
        QUrl fileUrl = media.canonicalUrl();
#else
        QUrl fileUrl = media.request().url();
#endif // QT_VERSION < QT_VERSION_CHECK(5, 14, 0)

        ui->titleLabel->setText(fileUrl.fileName());
        ui->titleLabel->setToolTip(fileUrl.fileName());

        if (fileUrl.isLocalFile()) {
            QString filePath(fileUrl.toLocalFile());
            QString suffix(filePath.mid(filePath.lastIndexOf('.') + 1));
            suffix = suffix.toUpper();

            TagLib::FileRef fileRef(filePath.toLocal8Bit().data());

            if (!fileRef.isNull() && fileRef.audioProperties()) {
                TagLib::AudioProperties *prop = fileRef.audioProperties();
                setAudioPropertyInfoForDisplay(prop->sampleRate(), prop->bitrate(), prop->channels(), suffix);
            }

            if (!fileRef.isNull() && fileRef.tag()) {
                TagLib::Tag * tag = fileRef.tag();
                setAudioMetadataForDisplay(QString::fromStdString(tag->title().to8Bit(true)),
                                           QString::fromStdString(tag->artist().to8Bit(true)),
                                           QString::fromStdString(tag->album().to8Bit(true)));
            }

            using namespace spID3;
            using namespace spFLAC;

            if (suffix == "MP3") {
                if (spID3::loadPictureData(filePath.toLocal8Bit().data())) {
                    QByteArray picData((const char*)spID3::getPictureDataPtr(), spID3::getPictureLength());
                    ui->coverLabel->setPixmap(QPixmap::fromImage(QImage::fromData(picData)));
                    spID3::freePictureData();
                }
            } else if (suffix == "FLAC") {
                if (spFLAC::loadPictureData(filePath.toLocal8Bit().data())) {
                    QByteArray picData((const char*)spFLAC::getPictureDataPtr(), spFLAC::getPictureLength());
                    ui->coverLabel->setPixmap(QPixmap::fromImage(QImage::fromData(picData)));
                    spFLAC::freePictureData();
                }
            }
        }
    });

    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, [=](qint64 pos) {
        ui->nowTimeLabel->setText(ms2str(pos));
        if (m_mediaPlayer->duration() != 0) {
            ui->playbackSlider->setSliderPosition(ui->playbackSlider->maximum() * pos / m_mediaPlayer->duration());
        }
    });

    connect(m_mediaPlayer, &QMediaPlayer::mutedChanged, this, [=](bool muted) {
        if (muted) {
            ui->volumeBtn->setIcon(QIcon(":/icons/icons/audio-volume-muted.png"));
        } else {
            ui->volumeBtn->setIcon(QIcon(":/icons/icons/audio-volume-high.png"));
        }
    });

    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, [=](qint64 dua) {
        ui->totalTimeLabel->setText(ms2str(dua));
    });

    connect(m_mediaPlayer, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State newState) {
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

    connect(m_mediaPlayer, &QMediaPlayer::volumeChanged, this, [=](int vol) {
        ui->volumeSlider->setValue(vol);
    });

    connect(m_mediaPlayer, static_cast<void(QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error),
            this, [=](QMediaPlayer::Error error) {
        switch (error) {
        default:
            break;
        }
        qDebug("%s aaaaaaaaaaaaa", m_mediaPlayer->errorString().toUtf8().data());
    });
}

void MainWindow::on_playbackModeBtn_clicked()
{
    QMediaPlaylist * playlist = m_mediaPlayer->playlist();
    if (!playlist) return;

    switch (playlist->playbackMode()) {
    case QMediaPlaylist::CurrentItemInLoop:
        playlist->setPlaybackMode(QMediaPlaylist::Loop);
        break;
    case QMediaPlaylist::Loop:
        playlist->setPlaybackMode(QMediaPlaylist::Sequential);
        break;
    case QMediaPlaylist::Sequential:
        playlist->setPlaybackMode(QMediaPlaylist::Random);
        break;
    case QMediaPlaylist::Random:
        playlist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
        break;
    default:
        break;
    }
}
