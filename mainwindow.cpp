#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QPainter>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QPropertyAnimation>
#include <QFileDialog>
#include <QTime>
#include <QStyle>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_mediaPlayer(new QMediaPlayer(this))
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

void MainWindow::loadFile()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
                                                      tr("Select songs to play"),
                                                      QDir::homePath(),
                                                      tr("Audio Files") + " (*.mp3 *.wav *.aiff *.ape *.flac *.ogg *.oga)");
    QMediaPlaylist * playlist = new QMediaPlaylist(m_mediaPlayer);
    playlist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
    for (const QString & fileName : files) {
        bool succ = playlist->addMedia(QMediaContent(QUrl::fromLocalFile(fileName)));
        if (!succ) {
            qDebug("!!!!!!!!! break point time !!!!!!!!!");
        }
    }

    m_mediaPlayer->setPlaylist(playlist);
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
}

void MainWindow::initConnections()
{
    connect(m_mediaPlayer, &QMediaPlayer::currentMediaChanged, this, [=](const QMediaContent &media) {
        ui->titleLabel->setText(media.request().url().fileName());
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
        qDebug(m_mediaPlayer->errorString().toUtf8() + "aaaaaaaaaaaaa");
    });
}
