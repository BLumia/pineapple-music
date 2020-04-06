#pragma once

#include <QSlider>
#include <QMouseEvent>

class SeekableSlider : public QSlider
{
    Q_OBJECT
public:
    explicit SeekableSlider(QWidget *parent = nullptr);

signals:

public slots:

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};

