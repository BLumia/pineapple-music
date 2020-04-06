#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }

class QPropertyAnimation;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *) override;
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void on_closeWindowBtn_clicked();

private:
    QPoint m_oldMousePos;
    bool m_clickedOnWindow = false;
    QLinearGradient m_bgLinearGradient;

    Ui::MainWindow *ui;

    QPropertyAnimation *m_fadeOutAnimation;
};
#endif // MAINWINDOW_H
