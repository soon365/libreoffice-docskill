/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include "QtFrameGrabber.hxx"
#include <QtFrameGrabber.moc>

#include <sal/log.hxx>
#include <vcl/filter/PngImageReader.hxx>
#include <vcl/graph.hxx>
#include <vcl/qt/QtUtils.hxx>
#include <vcl/scheduler.hxx>
#include <vcl/svapp.hxx>

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>

using namespace ::com::sun::star;

namespace
{
uno::Reference<css::graphic::XGraphic> toXGraphic(const QImage& rImage)
{
    SolarMutexGuard g;

    QByteArray aData;
    QBuffer aBuffer(&aData);
    rImage.save(&aBuffer, "PNG");

    SvMemoryStream aStream(aData.data(), aData.size(), StreamMode::READ);
    vcl::PngImageReader aReader(aStream);

    Bitmap aBitmap;
    if (aReader.read(aBitmap))
        return Graphic(aBitmap).GetXGraphic();

    return nullptr;
}
}

namespace avmedia::qt
{
namespace grabhelper
{
CaptureFrameHelperTaskWorker::CaptureFrameHelperTaskWorker(const QUrl& url)
    : QObject(nullptr)
    , m_url(url)
{
}

void CaptureFrameHelperTaskWorker::start()
{
    m_xMediaPlayer = std::make_unique<QMediaPlayer>();
    m_xVideoSink = std::make_unique<QVideoSink>();
    m_xMediaPlayer->setVideoSink(m_xVideoSink.get());

    connect(m_xMediaPlayer.get(), &QMediaPlayer::mediaStatusChanged, this,
            [this](const QMediaPlayer::MediaStatus& status) {
                if (status == QMediaPlayer::MediaStatus::LoadedMedia)
                {
                    if (!m_xMediaPlayer->hasVideo())
                    {
                        dataReady();
                    }
                }
            });

    connect(m_xMediaPlayer.get(), &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error error, const QString& errorString) {
                m_workerTaskResult.error = errorString;
                m_workerTaskResult.errorCode = error;
                dataReady();
            });

    connect(m_xVideoSink.get(), &QVideoSink::videoFrameChanged, this,
            [this](const QVideoFrame& vframe) {
                m_workerTaskResult.frame = vframe;
                dataReady();
            },
            Qt::SingleShotConnection);

    m_xMediaPlayer->setSource(m_url);
    m_xMediaPlayer->setPosition(m_mediaPos);
    m_xMediaPlayer->play();
}

void CaptureFrameHelperTaskWorker::setPos(double pos) { m_mediaPos = pos; }

void CaptureFrameHelperTaskWorker::dataReady()
{
    m_xMediaPlayer->stop();
    Q_EMIT ready(m_workerTaskResult);
}

CaptureFrameHelperResultHandler::CaptureFrameHelperResultHandler(std::condition_variable& cv,
                                                                 std::mutex& helperMutex)
    : QObject(nullptr)
    , m_cv(cv)
    , m_helperMutex(helperMutex){};
void CaptureFrameHelperResultHandler::handle(const LoadingDataResult& data)
{
    std::unique_lock<std::mutex> lock(m_helperMutex);
    m_workResult = data;
    m_cv.notify_all();
}

LoadingDataResult CaptureFrameHelperResultHandler::getData() { return m_workResult; }

CaptureFrameHelper::CaptureFrameHelper(const QUrl& url, double pos)
{
    m_helper = new CaptureFrameHelperTaskWorker(url);
    m_helper->setPos(pos);
    m_resultHandler = new CaptureFrameHelperResultHandler(m_cond, m_helperMutex);
    m_helper->moveToThread(&m_helperThread);
    m_resultHandler->moveToThread(&m_helperThread);

    QObject::connect(&m_helperThread, &QThread::started, m_helper,
                     &CaptureFrameHelperTaskWorker::start);
    QObject::connect(&m_helperThread, &QThread::finished, m_helper, &QObject::deleteLater);
    QObject::connect(&m_helperThread, &QThread::finished, m_resultHandler, &QObject::deleteLater);
    QObject::connect(m_helper, &CaptureFrameHelperTaskWorker::ready, m_resultHandler,
                     &CaptureFrameHelperResultHandler::handle);
}

CaptureFrameHelper::~CaptureFrameHelper()
{
    m_helperThread.quit();
    m_helperThread.wait();
}

LoadingDataResult CaptureFrameHelper::startAndGet()
{
    std::unique_lock<std::mutex> aLock(m_helperMutex);
    m_helperThread.start();
    m_cond.wait(aLock);
    return m_resultHandler->getData();
}
}
QtFrameGrabber::QtFrameGrabber(const QUrl& rSourceUrl) { m_url = rSourceUrl; }

css::uno::Reference<css::graphic::XGraphic> SAL_CALL QtFrameGrabber::grabFrame(double fMediaTime)
{
    grabhelper::CaptureFrameHelper aFrameHelper(m_url, fMediaTime * 1000);
    auto res = aFrameHelper.startAndGet();
    if (res.errorCode)
    {
        SAL_WARN("avmedia", "Media playback error occurred when trying to grab frame: "
                                << toOUString(res.error) << ", code: " << res.errorCode);
    }
    if (res.frame.has_value())
    {
        return toXGraphic(res.frame.value().toImage());
    }
    return nullptr;
}

}; // namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
