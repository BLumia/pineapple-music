// SPDX-FileCopyrightText: 2024 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "fftspectrum.h"

#include <QAudioBuffer>
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QAudioBufferOutput>
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QPainter>

#include <kissfft.hh>

FFTSpectrum::FFTSpectrum(QWidget* parent)
    : QWidget(parent)
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    , m_audioBufferOutput(new QAudioBufferOutput(this))
#endif // #if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    connect(this, &FFTSpectrum::mediaPlayerChanged, this, [=]() {
        if (m_mediaPlayer) {
            m_mediaPlayer->setAudioBufferOutput(m_audioBufferOutput);
        }
    });

    connect(m_audioBufferOutput, &QAudioBufferOutput::audioBufferReceived, this, [=](const QAudioBuffer& buffer) {
        const QAudioFormat& fmt = buffer.format();
        const QAudioFormat::SampleFormat sampleFormat = fmt.sampleFormat();
        QAudioFormat::ChannelConfig channelConfig = fmt.channelConfig();
        const QFlags supportedChannelConfig({ QAudioFormat::ChannelConfigMono, QAudioFormat::ChannelConfigStereo });
        const int frameCount = buffer.frameCount();
        kissfft<float> fft(frameCount, false);
        std::vector<kissfft<float>::cpx_t> samples(frameCount);
        std::vector<kissfft<float>::cpx_t> mass(frameCount);
        if (sampleFormat == QAudioFormat::Int16 && supportedChannelConfig.testFlag(channelConfig)) {
            if (channelConfig == QAudioFormat::ChannelConfigMono) {
                const QAudioBuffer::S16M* data = buffer.constData<QAudioBuffer::S16M>();
                for (int i = 0; i < frameCount; ++i) {
                    samples[i].real(data[i].value(QAudioFormat::FrontCenter) / float(32768));
                    samples[i].imag(0);
                }
            } else {
                const QAudioBuffer::S16S* data = buffer.constData<QAudioBuffer::S16S>();
                for (int i = 0; i < frameCount; ++i) {
                    samples[i].real(data[i].value(QAudioFormat::FrontLeft) / float(32768));
                    samples[i].imag(0);
                }
            }
        } else if (sampleFormat == QAudioFormat::Int32 && supportedChannelConfig.testFlag(channelConfig)) {
            if (channelConfig == QAudioFormat::ChannelConfigMono) {
                const QAudioBuffer::S32M* data = buffer.constData<QAudioBuffer::S32M>();
                for (int i = 0; i < frameCount; ++i) {
                    samples[i].real(data[i].value(QAudioFormat::FrontCenter) / float(2147483647));
                    samples[i].imag(0);
                }
            } else {
                const QAudioBuffer::S32S* data = buffer.constData<QAudioBuffer::S32S>();
                for (int i = 0; i < frameCount; ++i) {
                    samples[i].real(data[i].value(QAudioFormat::FrontLeft) / float(2147483647));
                    samples[i].imag(0);
                }
            }
        } else if (sampleFormat == QAudioFormat::Float && supportedChannelConfig.testFlag(channelConfig)) {
            if (channelConfig == QAudioFormat::ChannelConfigMono) {
                const QAudioBuffer::F32M* data = buffer.constData<QAudioBuffer::F32M>();
                for (int i = 0; i < frameCount; ++i) {
                    samples[i].real(data[i].value(QAudioFormat::FrontCenter));
                    samples[i].imag(0);
                }
            } else {
                const QAudioBuffer::F32S* data = buffer.constData<QAudioBuffer::F32S>();
                for (int i = 0; i < frameCount; ++i) {
                    samples[i].real(data[i].value(QAudioFormat::FrontLeft));
                    samples[i].imag(0);
                }
            }
        } else {
            qWarning() << "Unsupported format or channel config:" << sampleFormat << channelConfig;
            return;
        }
        fft.transform(samples.data(), mass.data());
        m_freq.resize(frameCount);
        for (int i = 0; i < frameCount; i++) {
            m_freq[i] = sqrt(mass[i].real() * mass[i].real() + mass[i].imag() * mass[i].imag());
        }
        for (auto& val : m_freq) {
            val = log10(val + 1);
        }
        update();
    });
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    resize(490, 420);
}

FFTSpectrum::~FFTSpectrum()
{

}

void FFTSpectrum::setMediaPlayer(QMediaPlayer* player)
{
    m_mediaPlayer = player;
    emit mediaPlayerChanged();
}

void FFTSpectrum::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);

    if (!m_freq.empty()) {
        int width = this->width();
        int height = this->height();
        int barWidth = std::max(1ULL, width / m_freq.size());
        for (int i = 0; i < m_freq.size(); i++) {
            int barHeight = static_cast<int>(sqrt(m_freq[i]) * height * 0.5);
            painter.fillRect(i * barWidth, height - barHeight, barWidth, barHeight, QColor(70, 130, 180, (int)(140 * m_freq[i]) + 90));
        }
    }
}


