/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>

#include <comphelper/compbase.hxx>

#include <com/sun/star/graphic/XGraphic.hpp>
#include <com/sun/star/media/XFrameGrabber.hpp>

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>

#include <condition_variable>
#include <mutex>

namespace avmedia::qt
{
namespace grabhelper
{
struct LoadingDataResult
{
    std::optional<QVideoFrame> frame;
    QMediaPlayer::Error errorCode;
    QString error;
};

class CaptureFrameHelperTaskWorker : public QObject
{
    Q_OBJECT
public:
    explicit CaptureFrameHelperTaskWorker(const QUrl& url);
    void start();
    void setPos(double pos);
Q_SIGNALS:
    void ready(const avmedia::qt::grabhelper::LoadingDataResult&);

private:
    void dataReady();
    double m_mediaPos = 0;
    LoadingDataResult m_workerTaskResult;
    QUrl m_url;
    std::unique_ptr<QMediaPlayer> m_xMediaPlayer;
    std::unique_ptr<QVideoSink> m_xVideoSink;
};

class CaptureFrameHelperResultHandler : public QObject
{
    Q_OBJECT
public:
    explicit CaptureFrameHelperResultHandler(std::condition_variable& cv, std::mutex& helperMutex);
public Q_SLOTS:
    void handle(const avmedia::qt::grabhelper::LoadingDataResult& data);

public:
    LoadingDataResult getData();

private:
    LoadingDataResult m_workResult;
    std::condition_variable& m_cv;
    std::mutex& m_helperMutex;
};

class CaptureFrameHelper
{
public:
    CaptureFrameHelper(const QUrl& url, double pos);
    ~CaptureFrameHelper();
    LoadingDataResult startAndGet();

private:
    std::condition_variable m_cond;
    std::mutex m_helperMutex;
    CaptureFrameHelperResultHandler* m_resultHandler;
    CaptureFrameHelperTaskWorker* m_helper;
    QThread m_helperThread;
};
}

class QtFrameGrabber : public QObject, public ::cppu::WeakImplHelper<css::media::XFrameGrabber>
{
    Q_OBJECT

private:
    QUrl m_url;

public:
    QtFrameGrabber(const QUrl& rSourceUrl);

    virtual css::uno::Reference<css::graphic::XGraphic>
        SAL_CALL grabFrame(double fMediaTime) override;
};

} // namespace avmedia::qt

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
