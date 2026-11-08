#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QVector>

class WaveformView : public QWidget
{
    Q_OBJECT

public:
    explicit WaveformView(QWidget *parent = nullptr);

    void setCurve(const QVector<double> &accelG, double durationMs, double targetG);
    void setShown(int count);
    void clearCurve();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> m_accel;
    int m_shown = 0;
    double m_durationMs = 20.0;
    double m_targetG = 50.0;
};
