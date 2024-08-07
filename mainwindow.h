#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVariant>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }

class QMediaPlayer;
class QAudioOutput;
class QPropertyAnimation;
QT_END_NAMESPACE

class PlaylistModel;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void commandlinePlayAudioFiles(QStringList audioFiles);
    void loadPlaylistBySingleLocalFile(const QString &path);
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
    void centerWindow();
    void createPlaylist(QList<QUrl> urlList, int index = -1);

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

private:
    bool m_clickedOnWindow = false;
    bool m_playbackSliderPressed = false;
    QLinearGradient m_bgLinearGradient;

    Ui::MainWindow *ui;

    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    QPropertyAnimation *m_fadeOutAnimation;
    PlaylistModel *m_playlistModel = nullptr; // TODO: move playback logic to player.cpp

    void initUiAndAnimation();
    void initConnections();

    static QString ms2str(qint64 ms);
    static QList<QUrl> strlst2urllst(QStringList strlst);
};
#endif // MAINWINDOW_H
