#pragma once

#ifndef GPUUPSCALER_H
#define GPUUPSCALER_H

#include <QObject>
#include <QProcess>
#include <QUrl>

class GpuUpscaler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl inputImage READ inputImage WRITE setInputImage NOTIFY inputImageChanged)
    Q_PROPERTY(QUrl outputImage READ outputImage NOTIFY outputImageChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit GpuUpscaler(QObject *parent = nullptr);

    QUrl inputImage() const { return m_inputImage; }
    void setInputImage(const QUrl &url);

    QUrl outputImage() const { return m_outputImage; }
    bool busy() const { return m_busy; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE void startUpscale(int scale = 4);

signals:
    void inputImageChanged();
    void outputImageChanged();
    void busyChanged();
    void lastErrorChanged();
    void upscaleFinished(bool ok);

private:
    void setBusy(bool b);
    void setLastError(const QString &err);

    QUrl m_inputImage;
    QUrl m_outputImage;
    bool m_busy = false;
    QString m_lastError;
    QProcess m_proc;
};

#endif // GPUUPSCALER_H
