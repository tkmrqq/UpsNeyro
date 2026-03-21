#ifndef FRAMECAPTURE_H
#define FRAMECAPTURE_H

#include <QObject>
#include <QString>

class FrameCapture : public QObject
{
    Q_OBJECT
public:
    explicit FrameCapture(QObject *parent = nullptr);

    // Захватывает кадр из videoPath на позиции positionSec
    // Сохраняет в outImagePath (PNG)
    // Возвращает true если успешно
    bool captureFrame(const QString &videoPath,
                      double positionSec,
                      const QString &outImagePath,
                      QString &errorOut);
};

#endif // FRAMECAPTURE_H
