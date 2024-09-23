// SPDX-FileCopyrightText: 2024 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVariant>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }

class QMediaDevices;
class QMediaPlayer;
class QAudioOutput;
class QPropertyAnimation;
QT_END_NAMESPACE

class LrcBar;
class PlaylistManager;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum PlaybackMode {
        CurrentItemOnce,
        CurrentItemInLoop,
        Sequential,
    };
    Q_ENUM(PlaybackMode)

    Q_PROPERTY(PlaybackMode playbackMode MEMBER m_playbackMode NOTIFY playbackModeChanged)

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void commandlinePlayAudioFiles(QStringList audioFiles);
    void setAudioPropertyInfoForDisplay(int sampleRate, int bitrate, int channelCount, QString audioExt);
    void setAudioMetadataForDisplay(QString title, QString artist, QString album);

public slots:
    void localSocketPlayAudioFiles(QVariant audioFilesVariant);

protected:
    void closeEvent(QCloseEvent *) override;
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

    void loadFile();
    void loadByModelIndex(const QModelIndex &index);
    void play();

    void setSkin(QString imagePath, bool save);

    void centerWindow();

private slots:
    void on_playbackModeBtn_clicked();
    void on_closeWindowBtn_clicked();
    void on_playBtn_clicked();
    void on_volumeSlider_valueChanged(int value);
    void on_stopBtn_clicked();
    void on_playbackSlider_valueChanged(int value);
    void on_prevBtn_clicked();
    void on_nextBtn_clicked();
    void on_volumeBtn_clicked();
    void on_minimumWindowBtn_clicked();
    void on_setSkinBtn_clicked();
    void on_playListBtn_clicked();
    void on_playlistView_activated(const QModelIndex &index);
    void on_lrcBtn_clicked();
    void on_actionHelp_triggered();

signals:
    void playbackModeChanged(enum PlaybackMode mode);

private:
    bool m_clickedOnWindow = false;
    bool m_playbackSliderPressed = false;
    QLinearGradient m_bgLinearGradient;
    QPixmap m_skin;
    enum PlaybackMode m_playbackMode = CurrentItemInLoop;

    Ui::MainWindow *ui;

    QMediaDevices *m_mediaDevices;
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    LrcBar *m_lrcbar;
    QPropertyAnimation *m_fadeOutAnimation;
    PlaylistManager *m_playlistManager;

    void initUiAndAnimation();
    void initConnections();

    void loadSkinData();
    void saveSkinData();

    static QString ms2str(qint64 ms);
    static QList<QUrl> strlst2urllst(QStringList strlst);
};
#endif // MAINWINDOW_H
