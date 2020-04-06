#include "seekableslider.h"

SeekableSlider::SeekableSlider(QWidget *parent) :
    QSlider(parent)
{
    //关闭分段移动
    setSingleStep(0);
    setPageStep(0);
}

//点击Slider即可调节Value
//只写了横向模式的Slider……
void SeekableSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QSlider::mousePressEvent(event);
        double pos = event->pos().x() / (double)width();
        setValue(pos * (maximum() - minimum()) + minimum());
    }
}

void SeekableSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QSlider::mousePressEvent(event);
        double pos = event->pos().x() / (double)width();
        setValue(pos * (maximum() - minimum()) + minimum());
    }
}
