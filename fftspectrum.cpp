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
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
        if (!isVisible()) return;
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
        // Apply Hanning window to reduce spectral leakage
        for (int i = 0; i < frameCount; ++i) {
            float window = 0.5f * (1.0f - cos(2.0f * M_PI * i / (frameCount - 1)));
            samples[i].real(samples[i].real() * window);
        }
        fft.transform(samples.data(), mass.data());
        // Use only the first half of FFT result (positive frequencies)
        int spectrumSize = frameCount / 2;
        m_freq.resize(spectrumSize);
        for (int i = 0; i < spectrumSize; i++) {
            m_freq[i] = sqrt(mass[i].real() * mass[i].real() + mass[i].imag() * mass[i].imag());
        }

        // Remove DC component and very low frequencies (like most spectrum analyzers do)
        // DC component (0 Hz) is usually not musically relevant
        m_freq[0] = 0;

        // Optionally remove the first few bins (very low frequencies below ~20Hz)
        // Calculate how many bins correspond to frequencies below 20Hz
        int sampleRate = fmt.sampleRate();
        int lowFreqCutoff = std::min(3, (20 * frameCount) / (sampleRate * 2)); // Limit to first 3 bins max
        for (int i = 1; i <= lowFreqCutoff && i < spectrumSize; i++) {
            m_freq[i] *= (float(i) / float(lowFreqCutoff + 1)); // Gradual fade-in instead of hard cut
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
        int barWidth = std::max(1, width / static_cast<int>(m_freq.size()));

        for (size_t i = 0; i < m_freq.size(); i++) {
            // Use sqrt to compress the range similar to original, but safer
            int barHeight = static_cast<int>(sqrt(std::max(0.0f, m_freq[i])) * height * 0.5);
            barHeight = std::max(0, std::min(barHeight, height));

            // Calculate alpha based on frequency value, similar to original
            int alpha = std::min(255, static_cast<int>(140 * m_freq[i]) + 90);
            alpha = std::max(90, alpha);

            QColor color(70, 130, 180, alpha);
            painter.fillRect(static_cast<int>(i) * barWidth, height - barHeight, barWidth, barHeight, color);
        }
    }
}


