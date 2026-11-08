#include "waveform_view.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

WaveformView::WaveformView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(360, 220);
    setAutoFillBackground(false);
}

void WaveformView::setCurve(const QVector<double> &accelG, double durationMs, double targetG)
{
    m_accel = accelG;
    m_durationMs = durationMs > 0.0 ? durationMs : 1.0;
    m_targetG = targetG;
    m_shown = 0;
    update();
}

void WaveformView::setShown(int count)
{
    m_shown = qBound(0, count, m_accel.size());
    update();
}

void WaveformView::clearCurve()
{
    m_accel.clear();
    m_shown = 0;
    m_targetG = 0.0;
    update();
}

void WaveformView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor(QStringLiteral("#0A141E")));

    const QRect plot = rect().adjusted(52, 26, -14, -30);
    p.setPen(QColor(QStringLiteral("#1C3348")));
    for (int i = 0; i <= 4; ++i) {
        const int y = plot.top() + plot.height() * i / 4;
        const int x = plot.left() + plot.width() * i / 4;
        p.drawLine(plot.left(), y, plot.right(), y);
        p.drawLine(x, plot.top(), x, plot.bottom());
    }

    p.setPen(QColor(QStringLiteral("#9FB3C8")));
    p.drawText(QRect(8, 4, width() - 16, 20), Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("冲击加速度"));

    if (m_accel.isEmpty() || plot.width() < 8 || plot.height() < 8) {
        p.setPen(QColor(QStringLiteral("#6E8498")));
        p.drawText(plot, Qt::AlignCenter, QStringLiteral("等待冲击"));
        return;
    }

    double peak = qMax(10.0, qAbs(m_targetG));
    for (int i = 0; i < m_shown; ++i)
        peak = qMax(peak, qAbs(m_accel.at(i)));
    double yMax = peak * 1.15;
    double yMin = 0.0;
    if (m_targetG < 0.0) {
        yMax = 0.0;
        yMin = -peak * 1.15;
    }

    const auto yOf = [&](double g) {
        const double t = (g - yMin) / (yMax - yMin);
        return plot.bottom() - t * plot.height();
    };
    const int sampleCount = static_cast<int>(m_accel.size());
    const auto xOf = [&](int i) {
        if (sampleCount <= 1)
            return plot.left();
        return plot.left() + plot.width() * i / (sampleCount - 1);
    };

    QPen targetPen(QColor(QStringLiteral("#F5C518")));
    targetPen.setStyle(Qt::DashLine);
    targetPen.setWidth(1);
    p.setPen(targetPen);
    const int targetY = qRound(yOf(m_targetG));
    p.drawLine(plot.left(), targetY, plot.right(), targetY);

    p.setPen(QColor(QStringLiteral("#3D5166")));
    p.drawLine(plot.left(), qRound(yOf(0.0)), plot.right(), qRound(yOf(0.0)));

    QPainterPath path;
    const int n = qMin(m_shown, m_accel.size());
    for (int i = 0; i < n; ++i) {
        const QPointF pt(xOf(i), yOf(m_accel.at(i)));
        if (i == 0)
            path.moveTo(pt);
        else
            path.lineTo(pt);
    }
    QPen trace(QColor(QStringLiteral("#FFE600")));
    trace.setWidth(2);
    trace.setCosmetic(true);
    p.setPen(trace);
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);

    p.setPen(QColor(QStringLiteral("#9FB3C8")));
    p.drawText(QRect(0, targetY - 8, 48, 16), Qt::AlignRight | Qt::AlignVCenter,
               QString::number(m_targetG, 'f', 0));
    p.drawText(QRect(plot.left(), height() - 22, 40, 18), Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("0"));
    p.drawText(QRect(plot.right() - 70, height() - 22, 70, 18), Qt::AlignRight | Qt::AlignVCenter,
               QString::number(m_durationMs, 'f', 0) + QStringLiteral(" ms"));
}
