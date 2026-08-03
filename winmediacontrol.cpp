// SPDX-FileCopyrightText: 2026 Gary Wang <opensource@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "winmediacontrol.h"

#include <QDebug>
#include <QBuffer>

#ifdef Q_OS_WIN

#include <roapi.h>
#include <windows.media.h>
#include <windows.storage.streams.h>
#include <systemmediatransportcontrolsinterop.h>

#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Media;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage::Streams;

using ButtonPressedHandler = ITypedEventHandler<SystemMediaTransportControls*,
    SystemMediaTransportControlsButtonPressedEventArgs*>;
using PositionChangeHandler = ITypedEventHandler<SystemMediaTransportControls*,
    PlaybackPositionChangeRequestedEventArgs*>;

// ---------------------------------------------------------------------------
// COM event handler for button presses
// ---------------------------------------------------------------------------
class ButtonDelegate : public ButtonPressedHandler
{
public:
    ButtonDelegate(WinMediaControl* owner) : m_ref(1), m_owner(owner) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(ButtonPressedHandler)) {
            *ppv = static_cast<ButtonPressedHandler*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG r = InterlockedDecrement(&m_ref);
        if (r == 0) delete this;
        return r;
    }

    HRESULT STDMETHODCALLTYPE Invoke(
        ISystemMediaTransportControls*,
        ISystemMediaTransportControlsButtonPressedEventArgs* args) override
    {
        if (args) {
            SystemMediaTransportControlsButton button;
            if (SUCCEEDED(args->get_Button(&button))) {
                switch (button) {
                case SystemMediaTransportControlsButton_Play:
                    emit m_owner->playRequested();
                    break;
                case SystemMediaTransportControlsButton_Pause:
                    emit m_owner->pauseRequested();
                    break;
                case SystemMediaTransportControlsButton_Stop:
                    emit m_owner->stopRequested();
                    break;
                case SystemMediaTransportControlsButton_Next:
                    emit m_owner->nextRequested();
                    break;
                case SystemMediaTransportControlsButton_Previous:
                    emit m_owner->previousRequested();
                    break;
                default:
                    break;
                }
            }
        }
        return S_OK;
    }

private:
    LONG m_ref;
    WinMediaControl* m_owner;
};

// ---------------------------------------------------------------------------
// COM event handler for position change (seek) requests
// ---------------------------------------------------------------------------
class PositionChangeDelegate : public PositionChangeHandler
{
public:
    PositionChangeDelegate(WinMediaControl* owner) : m_ref(1), m_owner(owner) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(PositionChangeHandler)) {
            *ppv = static_cast<PositionChangeHandler*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG r = InterlockedDecrement(&m_ref);
        if (r == 0) delete this;
        return r;
    }

    HRESULT STDMETHODCALLTYPE Invoke(
        ISystemMediaTransportControls*,
        IPlaybackPositionChangeRequestedEventArgs* args) override
    {
        if (args) {
            TimeSpan ts;
            if (SUCCEEDED(args->get_RequestedPlaybackPosition(&ts))) {
                emit m_owner->seekRequested(ts.Duration / 10000);
            }
        }
        return S_OK;
    }

private:
    LONG m_ref;
    WinMediaControl* m_owner;
};

// ---------------------------------------------------------------------------
// WinMediaControlPrivate
// ---------------------------------------------------------------------------
class WinMediaControlPrivate
{
public:
    ComPtr<ISystemMediaTransportControls> smtc;
    ComPtr<ISystemMediaTransportControls2> smtc2;
    ComPtr<ISystemMediaTransportControlsDisplayUpdater> updater;
    ComPtr<IMusicDisplayProperties> musicProps;
    EventRegistrationToken buttonToken{};
    EventRegistrationToken positionChangeToken{};
    bool initialized = false;

    QString title;
    QString artist;
    QString album;
    QImage coverArt;
};

WinMediaControl::WinMediaControl(QObject* parent)
    : QObject(parent)
    , d(new WinMediaControlPrivate())
{
    HRESULT hr = RoInitialize(RO_INIT_SINGLETHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        qWarning() << "WinMediaControl: RoInitialize failed:" << Qt::hex << hr;
    }
}

WinMediaControl::~WinMediaControl()
{
    shutdown();
    delete d;
}

void WinMediaControl::initialize(void* hwnd)
{
    if (d->initialized || !hwnd) return;

    HWND window = static_cast<HWND>(hwnd);

    ComPtr<ISystemMediaTransportControlsInterop> interop;
    HRESULT hr = RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_Media_SystemMediaTransportControls).Get(),
        IID_PPV_ARGS(&interop));
    if (FAILED(hr) || !interop) {
        qWarning() << "WinMediaControl: Failed to get interop factory:" << Qt::hex << hr;
        return;
    }

    hr = interop->GetForWindow(window, IID_PPV_ARGS(&d->smtc));
    if (FAILED(hr) || !d->smtc) {
        qWarning() << "WinMediaControl: GetForWindow failed:" << Qt::hex << hr;
        return;
    }

    d->smtc->put_IsEnabled(true);
    d->smtc->put_IsPlayEnabled(true);
    d->smtc->put_IsPauseEnabled(true);
    d->smtc->put_IsStopEnabled(true);
    d->smtc->put_IsNextEnabled(true);
    d->smtc->put_IsPreviousEnabled(true);

    d->smtc.As(&d->smtc2);

    d->smtc->get_DisplayUpdater(&d->updater);
    if (d->updater) {
        d->updater->put_Type(MediaPlaybackType_Music);
        d->updater->get_MusicProperties(&d->musicProps);
        d->updater->Update();
    }

    auto* btnHandler = new ButtonDelegate(this);
    hr = d->smtc->add_ButtonPressed(btnHandler, &d->buttonToken);
    btnHandler->Release();

    if (d->smtc2) {
        auto* posHandler = new PositionChangeDelegate(this);
        hr = d->smtc2->add_PlaybackPositionChangeRequested(posHandler, &d->positionChangeToken);
        posHandler->Release();
    }

    d->initialized = true;
    qDebug() << "WinMediaControl: SMTC initialized successfully";
}

void WinMediaControl::shutdown()
{
    if (!d->initialized) return;

    if (d->smtc) {
        d->smtc->remove_ButtonPressed(d->buttonToken);
        if (d->smtc2) {
            d->smtc2->remove_PlaybackPositionChangeRequested(d->positionChangeToken);
        }
        d->smtc->put_IsEnabled(false);
    }
    d->smtc.Reset();
    d->smtc2.Reset();
    d->updater.Reset();
    d->musicProps.Reset();
    d->initialized = false;
}

static ComPtr<IRandomAccessStreamReference> imageToStreamReference(const QImage& image)
{
    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    ComPtr<IInspectable> insp;
    HRESULT hr = RoActivateInstance(
        HStringReference(RuntimeClass_Windows_Storage_Streams_InMemoryRandomAccessStream).Get(),
        &insp);
    if (FAILED(hr) || !insp) return nullptr;

    ComPtr<IRandomAccessStream> stream;
    insp.As(&stream);
    if (!stream) return nullptr;

    ComPtr<IOutputStream> outputStream;
    stream->GetOutputStreamAt(0, &outputStream);

    ComPtr<IDataWriterFactory> writerFactory;
    RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_Storage_Streams_DataWriter).Get(),
        IID_PPV_ARGS(&writerFactory));
    if (!writerFactory) return nullptr;

    ComPtr<IDataWriter> writer;
    writerFactory->CreateDataWriter(outputStream.Get(), &writer);
    if (!writer) return nullptr;

    writer->WriteBytes(
        static_cast<UINT32>(imageData.size()),
        reinterpret_cast<BYTE*>(imageData.data()));

    ComPtr<IAsyncOperation<UINT32>> storeOp;
    writer->StoreAsync(&storeOp);
    if (storeOp) {
        // For in-memory streams this completes synchronously; just drain it
        UINT32 result = 0;
        storeOp->GetResults(&result);
    }

    stream->Seek(0);

    ComPtr<IRandomAccessStreamReferenceStatics> rars;
    RoGetActivationFactory(
        HStringReference(RuntimeClass_Windows_Storage_Streams_RandomAccessStreamReference).Get(),
        IID_PPV_ARGS(&rars));
    if (!rars) return nullptr;

    ComPtr<IRandomAccessStreamReference> ref;
    rars->CreateFromStream(stream.Get(), &ref);
    return ref;
}

void WinMediaControl::setMetadata(const QString& title, const QString& artist,
                                  const QString& album)
{
    d->title = title;
    d->artist = artist;
    d->album = album;

    if (!d->initialized || !d->updater || !d->musicProps) return;

    d->updater->put_Type(MediaPlaybackType_Music);
    d->updater->get_MusicProperties(&d->musicProps);

    {
        std::wstring wstr = title.toStdWString();
        d->musicProps->put_Title(HStringReference(wstr.c_str()).Get());
    }
    {
        std::wstring wstr = artist.toStdWString();
        d->musicProps->put_Artist(HStringReference(wstr.c_str()).Get());
    }

    ComPtr<IMusicDisplayProperties2> musicProps2;
    if (SUCCEEDED(d->musicProps.As(&musicProps2)) && musicProps2) {
        std::wstring wstr = album.toStdWString();
        musicProps2->put_AlbumTitle(HStringReference(wstr.c_str()).Get());
    }

    if (!d->coverArt.isNull()) {
        auto thumbRef = imageToStreamReference(d->coverArt);
        if (thumbRef) {
            d->updater->put_Thumbnail(thumbRef.Get());
        }
    } else {
        d->updater->put_Thumbnail(nullptr);
    }

    d->updater->Update();
}

void WinMediaControl::setCoverArt(const QImage& coverArt)
{
    d->coverArt = coverArt;
    if (d->initialized) {
        setMetadata(d->title, d->artist, d->album);
    }
}

void WinMediaControl::setPlaybackState(int state)
{
    if (!d->initialized || !d->smtc) return;

    MediaPlaybackStatus status;
    switch (state) {
    case 0:  status = MediaPlaybackStatus_Stopped; break;
    case 1:  status = MediaPlaybackStatus_Playing;  break;
    case 2:  status = MediaPlaybackStatus_Paused;   break;
    default: status = MediaPlaybackStatus_Stopped;  break;
    }
    d->smtc->put_PlaybackStatus(status);
}

void WinMediaControl::setPosition(qint64 positionMs, qint64 durationMs)
{
    if (!d->initialized || !d->smtc2) return;

    ComPtr<IInspectable> insp;
    HRESULT hr = RoActivateInstance(
        HStringReference(RuntimeClass_Windows_Media_SystemMediaTransportControlsTimelineProperties).Get(),
        &insp);
    if (FAILED(hr) || !insp) return;

    ComPtr<ISystemMediaTransportControlsTimelineProperties> timeline;
    insp.As(&timeline);
    if (!timeline) return;

    timeline->put_StartTime({ 0 });
    timeline->put_EndTime({ durationMs * 10000 });
    timeline->put_Position({ positionMs * 10000 });
    timeline->put_MinSeekTime({ 0 });
    timeline->put_MaxSeekTime({ durationMs * 10000 });

    d->smtc2->UpdateTimelineProperties(timeline.Get());
}

void WinMediaControl::setControlsEnabled(bool play, bool pause, bool stop,
                                          bool next, bool previous)
{
    if (!d->initialized || !d->smtc) return;

    d->smtc->put_IsPlayEnabled(play);
    d->smtc->put_IsPauseEnabled(pause);
    d->smtc->put_IsStopEnabled(stop);
    d->smtc->put_IsNextEnabled(next);
    d->smtc->put_IsPreviousEnabled(previous);
}

#else // Q_OS_WIN

// Non-Windows stub implementations

WinMediaControl::WinMediaControl(QObject* parent)
    : QObject(parent)
    , d(nullptr)
{
}

WinMediaControl::~WinMediaControl() = default;

void WinMediaControl::initialize(void*) {}
void WinMediaControl::shutdown() {}

void WinMediaControl::setMetadata(const QString&, const QString&, const QString&) {}
void WinMediaControl::setCoverArt(const QImage&) {}
void WinMediaControl::setPlaybackState(int) {}
void WinMediaControl::setPosition(qint64, qint64) {}
void WinMediaControl::setControlsEnabled(bool, bool, bool, bool, bool) {}

#endif // Q_OS_WIN
