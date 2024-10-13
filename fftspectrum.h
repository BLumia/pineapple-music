// SPDX-FileCopyrightText: 2024 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>
#include <QMediaPlayer>

class QAudioBufferOutput;
class FFTSpectrum : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QMediaPlayer * mediaPlayer MEMBER m_mediaPlayer WRITE setMediaPlayer NOTIFY mediaPlayerChanged)
public:
    explicit FFTSpectrum(QWidget* parent);
    ~FFTSpectrum();

    void setMediaPlayer(QMediaPlayer* player);

signals:
    void mediaPlayerChanged();

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    QMediaPlayer* m_mediaPlayer = nullptr;
    QAudioBufferOutput* m_audioBufferOutput = nullptr;
    std::vector<float> m_freq;
};
