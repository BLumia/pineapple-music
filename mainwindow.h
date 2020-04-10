#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }

class QMediaPlayer;
class QMediaPlaylist;
class QPropertyAnimation;
QT_END_NAMESPACE

class PlaylistModel;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void commandlinePlayAudioFiles(QList<QUrl> audioFiles);
    void loadPlaylistBySingleLocalFile(const QString &path);

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
    QMediaPlaylist *createPlaylist(QList<QUrl> urlList);

private slots:
    void on_playbackModeBtn_clicked();

private slots:
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
    QPoint m_oldMousePos;
    bool m_clickedOnWindow = false;
    bool m_playbackSliderPressed = false;
    QLinearGradient m_bgLinearGradient;

    Ui::MainWindow *ui;

    QMediaPlayer *m_mediaPlayer;
    QPropertyAnimation *m_fadeOutAnimation;
    PlaylistModel *m_playlistModel = nullptr; // TODO: move playback logic to player.cpp

    void initUiAndAnimation();
    void initConnections();

    static QString ms2str(qint64 ms);
};
#endif // MAINWINDOW_H
