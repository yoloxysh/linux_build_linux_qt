#include "qt_demo.h"

#include "waveform_view.h"

#include <QtCore/QRandomGenerator>
#include <QtCore/QTime>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QTouchEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr int kCountMin = 1;
constexpr int kCountMax = 9999;
constexpr int kAccelMin = 0;
constexpr int kAccelMax = 2500;
constexpr int kIntervalMin = 1;
constexpr int kIntervalMax = 9999;
constexpr int kModeCounts[] = {100, 200, 300, 400, 500};

struct AirHammerTrace {
    QVector<double> accelG;
    double durationMs = 0.0;
    double peakG = 0.0;
};

double fastPulse(double t, double start, double riseMs, double fallMs, double amp, double decay)
{
    if (t < start || riseMs <= 0.0 || fallMs <= 0.0)
        return 0.0;
    const double dt = t - start;
    if (dt < riseMs) {
        const double x = dt / riseMs;
        return amp * std::sin(0.5 * kPi * x);
    }
    const double fall = dt - riseMs;
    if (fall >= fallMs)
        return 0.0;
    return amp * std::exp(-decay * fall / fallMs);
}

AirHammerTrace synthesizeAirHammer(double targetG)
{
    auto *rng = QRandomGenerator::global();
    const double totalMs = 8.0;
    const int samples = 360;
    const double preAmp = 0.20;
    const double postAmp = 0.20;
    const double preRise = 0.12;
    const double preFall = 0.26;
    const double mainRise = 0.16;
    const double mainFall = 0.30;
    const double postRise = 0.10;
    const double postFall = 0.24;
    const double preDecay = 6.5;
    const double mainDecay = 6.5;
    const double postDecay = 6.5;
    const double preStart = 2.40;
    const double mainStart = 3.10;
    const double postStart = 3.86;
    const double desiredPeak = targetG <= 0.0 ? 0.0 : targetG * (0.97 + 0.06 * rng->generateDouble());

    AirHammerTrace trace;
    trace.durationMs = totalMs;
    trace.accelG.resize(samples);
    QVector<double> shape(samples);
    int peakIndex = 0;
    for (int i = 0; i < samples; ++i) {
        const double t = totalMs * i / (samples - 1);
        double g = fastPulse(t, preStart, preRise, preFall, preAmp, preDecay);
        g += fastPulse(t, mainStart, mainRise, mainFall, 1.0, mainDecay);
        g += fastPulse(t, postStart, postRise, postFall, postAmp, postDecay);
        const double buzz = 0.006 + 0.010 * g;
        g += buzz * (rng->generateDouble() * 2.0 - 1.0);
        g += 0.004 * std::sin(2.0 * kPi * (18.0 + 10.0 * rng->generateDouble()) * t + rng->generateDouble());
        shape[i] = qMax(0.0, g);
        if (shape[i] > shape[peakIndex])
            peakIndex = i;
    }

    const auto sampleAt = [&](double t) {
        return qBound(0, qRound(t / totalMs * (samples - 1)), samples - 1);
    };
    const int prePeakIndex = sampleAt(preStart + preRise);
    const int postPeakIndex = sampleAt(postStart + postRise);
    const int burrs = 7 + rng->bounded(6);
    for (int n = 0; n < burrs; ++n) {
        const int at = 4 + rng->bounded(samples - 8);
        if (qAbs(at - peakIndex) < 5 || qAbs(at - prePeakIndex) < 4 || qAbs(at - postPeakIndex) < 4)
            continue;
        const double spike = (0.012 + 0.028 * rng->generateDouble()) * (rng->generateDouble() < 0.25 ? -0.6 : 1.0);
        shape[at] = qMax(0.0, shape[at] + spike);
        if (at + 1 < samples)
            shape[at + 1] = qMax(0.0, shape[at + 1] + spike * 0.35);
    }
    for (int i = 0; i < samples; ++i) {
        if (qAbs(i - peakIndex) > 4)
            shape[i] = qMin(shape[i], 0.32);
    }

    const double peakValue = qMax(shape[peakIndex], 0.2);
    const double scale = desiredPeak / peakValue;
    for (int i = 0; i < samples; ++i)
        trace.accelG[i] = shape[i] * scale;
    trace.peakG = desiredPeak;
    return trace;
}

QWidget *addMetric(QWidget *parent, const QString &caption, QLabel **value)
{
    auto *box = new QFrame(parent);
    box->setObjectName(QStringLiteral("metric"));
    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(0);
    auto *cap = new QLabel(caption, box);
    cap->setObjectName(QStringLiteral("metricCap"));
    *value = new QLabel(QStringLiteral("--"), box);
    (*value)->setObjectName(QStringLiteral("metricVal"));
    layout->addWidget(cap);
    layout->addWidget(*value);
    return box;
}

QLineEdit *makeNumberEdit(QWidget *parent, int value)
{
    auto *edit = new QLineEdit(parent);
    edit->setText(QString::number(value));
    edit->setAlignment(Qt::AlignCenter);
    edit->setInputMethodHints(Qt::ImhDigitsOnly | Qt::ImhPreferNumbers | Qt::ImhNoPredictiveText);
    edit->setMaxLength(4);
    return edit;
}

QPushButton *makeStepButton(QWidget *parent, const QString &text)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("step"));
    return button;
}

} // namespace

qt_demo::qt_demo(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("bench"));
    setWindowTitle(QStringLiteral("冲击试验台"));
    buildUi();
    applyStyle();
    qApp->installEventFilter(this);
    setState(Idle);
    updateTestTime();

    connect(&m_clockTimer, &QTimer::timeout, this, &qt_demo::updateClock);
    m_clockTimer.start(1000);
    updateClock();

    m_replayTimer.setInterval(16);
    connect(&m_replayTimer, &QTimer::timeout, this, &qt_demo::tickReplay);
    m_intervalTimer.setSingleShot(true);
    connect(&m_intervalTimer, &QTimer::timeout, this, &qt_demo::beginPulse);

    connect(m_mode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int index) { applyMode(index); });
    connect(m_countUp, &QPushButton::clicked, this, [this] { stepCount(1); });
    connect(m_countDown, &QPushButton::clicked, this, [this] { stepCount(-1); });
    connect(m_accelUp, &QPushButton::clicked, this, [this] { stepAccel(1); });
    connect(m_accelDown, &QPushButton::clicked, this, [this] { stepAccel(-1); });
    connect(m_intervalUp, &QPushButton::clicked, this, [this] { stepInterval(1); });
    connect(m_intervalDown, &QPushButton::clicked, this, [this] { stepInterval(-1); });
    connect(m_start, &QPushButton::clicked, this, [this] {
        if (m_state == Paused)
            resume();
        else
            startTest();
    });
    connect(m_pause, &QPushButton::clicked, this, [this] {
        if (m_state != IntervalWait)
            return;
        m_intervalLeftMs = m_intervalTimer.remainingTime();
        m_intervalTimer.stop();
        if (m_runClockActive) {
            m_elapsedMs += m_runClock.elapsed();
            m_runClockActive = false;
        }
        setState(Paused);
        updateTestTime();
    });
    connect(m_stop, &QPushButton::clicked, this, [this] { stopTest(Stopped); });
    connect(m_reset, &QPushButton::clicked, this, &qt_demo::resetTest);
    connect(m_estop, &QPushButton::clicked, this, [this] { stopTest(Estop); });
}

void qt_demo::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 6, 8, 6);
    root->setSpacing(6);

    auto *top = new QHBoxLayout;
    auto *title = new QLabel(QStringLiteral("冲击试验台"), this);
    title->setObjectName(QStringLiteral("title"));
    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("status"));
    m_status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_clock = new QLabel(this);
    m_clock->setObjectName(QStringLiteral("clock"));
    m_clock->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_clock->setMinimumWidth(72);
    top->addWidget(title);
    top->addStretch(1);
    top->addWidget(m_status);
    top->addSpacing(12);
    top->addWidget(m_clock);
    root->addLayout(top);

    auto *mid = new QHBoxLayout;
    mid->setSpacing(8);

    auto *panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("panel"));
    panel->setFixedWidth(300);
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(8, 6, 8, 6);
    panelLayout->setSpacing(4);

    auto *modeCap = new QLabel(QStringLiteral("测试模式"), panel);
    modeCap->setObjectName(QStringLiteral("fieldCap"));
    m_mode = new QComboBox(panel);
    m_mode->addItems({
        QStringLiteral("模式测试1"),
        QStringLiteral("模式测试2"),
        QStringLiteral("模式测试3"),
        QStringLiteral("模式测试4"),
        QStringLiteral("模式测试5")
    });
    panelLayout->addWidget(modeCap);
    panelLayout->addWidget(m_mode);

    auto *countCap = new QLabel(QStringLiteral("冲击次数"), panel);
    countCap->setObjectName(QStringLiteral("fieldCap"));
    auto *countRow = new QHBoxLayout;
    countRow->setSpacing(4);
    m_countEdit = makeNumberEdit(panel, m_countValue);
    m_countEdit->installEventFilter(this);
    m_countUp = makeStepButton(panel, QStringLiteral("▲"));
    m_countDown = makeStepButton(panel, QStringLiteral("▼"));
    countRow->addWidget(m_countEdit, 1);
    countRow->addWidget(m_countUp);
    countRow->addWidget(m_countDown);
    panelLayout->addWidget(countCap);
    panelLayout->addLayout(countRow);

    auto *accelCap = new QLabel(QStringLiteral("冲击加速度"), panel);
    accelCap->setObjectName(QStringLiteral("fieldCap"));
    auto *accelRow = new QHBoxLayout;
    accelRow->setSpacing(4);
    m_accelEdit = makeNumberEdit(panel, m_accelValue);
    m_accelEdit->installEventFilter(this);
    m_accelUp = makeStepButton(panel, QStringLiteral("▲"));
    m_accelDown = makeStepButton(panel, QStringLiteral("▼"));
    auto *accelUnit = new QLabel(QStringLiteral("g"), panel);
    accelUnit->setObjectName(QStringLiteral("unit"));
    accelRow->addWidget(m_accelEdit, 1);
    accelRow->addWidget(m_accelUp);
    accelRow->addWidget(m_accelDown);
    accelRow->addWidget(accelUnit);
    panelLayout->addWidget(accelCap);
    panelLayout->addLayout(accelRow);

    auto *intervalCap = new QLabel(QStringLiteral("冲击间隔"), panel);
    intervalCap->setObjectName(QStringLiteral("fieldCap"));
    auto *intervalRow = new QHBoxLayout;
    intervalRow->setSpacing(4);
    m_intervalEdit = makeNumberEdit(panel, m_intervalValue);
    m_intervalEdit->installEventFilter(this);
    m_intervalUp = makeStepButton(panel, QStringLiteral("▲"));
    m_intervalDown = makeStepButton(panel, QStringLiteral("▼"));
    auto *intervalUnit = new QLabel(QStringLiteral("秒"), panel);
    intervalUnit->setObjectName(QStringLiteral("unit"));
    intervalRow->addWidget(m_intervalEdit, 1);
    intervalRow->addWidget(m_intervalUp);
    intervalRow->addWidget(m_intervalDown);
    intervalRow->addWidget(intervalUnit);
    panelLayout->addWidget(intervalCap);
    panelLayout->addLayout(intervalRow);

    auto *hint = new QLabel(QStringLiteral("间隔按秒设置。底部是剩余时间，单位分钟。"), panel);
    hint->setObjectName(QStringLiteral("hint"));
    hint->setWordWrap(true);
    panelLayout->addWidget(hint);
    panelLayout->addStretch(1);

    auto *buttons = new QGridLayout;
    buttons->setSpacing(4);
    m_start = new QPushButton(QStringLiteral("开始"), panel);
    m_pause = new QPushButton(QStringLiteral("暂停"), panel);
    m_stop = new QPushButton(QStringLiteral("停止"), panel);
    m_reset = new QPushButton(QStringLiteral("复位"), panel);
    m_start->setObjectName(QStringLiteral("start"));
    buttons->addWidget(m_start, 0, 0);
    buttons->addWidget(m_pause, 0, 1);
    buttons->addWidget(m_stop, 1, 0);
    buttons->addWidget(m_reset, 1, 1);
    panelLayout->addLayout(buttons);
    mid->addWidget(panel);

    m_wave = new WaveformView(this);
    mid->addWidget(m_wave, 1);
    root->addLayout(mid, 1);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 1);
    m_progress->setValue(0);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(8);
    root->addWidget(m_progress);

    auto *bottom = new QHBoxLayout;
    bottom->setSpacing(6);
    bottom->addWidget(addMetric(this, QStringLiteral("测试进度"), &m_countText), 1);
    bottom->addWidget(addMetric(this, QStringLiteral("峰值加速度"), &m_peakText), 1);
    bottom->addWidget(addMetric(this, QStringLiteral("剩余时间"), &m_timeText), 1);
    m_estop = new QPushButton(QStringLiteral("急停"), this);
    m_estop->setObjectName(QStringLiteral("estop"));
    bottom->addWidget(m_estop);
    root->addLayout(bottom);

    buildPad();
}

void qt_demo::buildPad()
{
    m_pad = new QFrame(this);
    m_pad->setObjectName(QStringLiteral("pad"));
    auto *grid = new QGridLayout(m_pad);
    grid->setContentsMargins(8, 8, 8, 8);
    grid->setSpacing(4);
    const QStringList keys = {
        QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"),
        QStringLiteral("4"), QStringLiteral("5"), QStringLiteral("6"),
        QStringLiteral("7"), QStringLiteral("8"), QStringLiteral("9"),
        QStringLiteral("删除"), QStringLiteral("0"), QStringLiteral("清空")
    };
    for (int i = 0; i < keys.size(); ++i) {
        auto *key = new QPushButton(keys.at(i), m_pad);
        key->setObjectName(QStringLiteral("padKey"));
        grid->addWidget(key, i / 3, i % 3);
        connect(key, &QPushButton::clicked, this, [this, key] {
            const QString text = key->text();
            if (text == QStringLiteral("删除")) {
                if (m_padEdit)
                    m_padEdit->backspace();
            } else if (text == QStringLiteral("清空")) {
                if (m_padEdit)
                    m_padEdit->clear();
            } else {
                appendDigit(text);
            }
        });
    }
    auto *ok = new QPushButton(QStringLiteral("确定"), m_pad);
    ok->setObjectName(QStringLiteral("padOk"));
    grid->addWidget(ok, 4, 0, 1, 3);
    connect(ok, &QPushButton::clicked, this, [this] {
        commitEdits();
        hidePad();
    });
    m_pad->hide();
}

void qt_demo::applyStyle()
{
    setStyleSheet(QStringLiteral(
        "QWidget#bench { background: #071018; color: #E7EEF5; }"
        "QLabel { background: transparent; color: #E7EEF5; }"
        "QLabel#title { font-size: 18px; font-weight: 700; color: #FFE14A; }"
        "QLabel#status { font-size: 15px; font-weight: 700; }"
        "QLabel#clock, QLabel#hint { color: #8AA4BA; font-size: 12px; }"
        "QLabel#fieldCap { color: #9FB3C8; font-size: 12px; }"
        "QLabel#unit { color: #FFE14A; font-size: 14px; font-weight: 700; min-width: 16px; }"
        "QFrame#panel, QFrame#metric, QFrame#pad { background: #12202E; border: 1px solid #234056; border-radius: 4px; }"
        "QLabel#metricCap { color: #8AA4BA; font-size: 12px; }"
        "QLabel#metricVal { color: #FFE14A; font-size: 16px; font-weight: 700; }"
        "QLineEdit, QComboBox { background: #0B1722; color: #FFE14A; border: 1px solid #2C5270;"
        "  border-radius: 3px; min-height: 30px; font-size: 16px; padding: 1px 6px; }"
        "QLineEdit:focus, QComboBox:focus { border: 1px solid #FFE14A; }"
        "QLineEdit:disabled, QComboBox:disabled { color: #6C7C8A; }"
        "QComboBox QAbstractItemView { background: #12202E; color: #E7EEF5; selection-background-color: #1F4E79; }"
        "QPushButton { background: #1A3A56; color: #E7EEF5; border: 1px solid #2F6FA3;"
        "  border-radius: 3px; min-height: 28px; font-size: 14px; }"
        "QPushButton:disabled { background: #1A242E; color: #6C7C8A; border-color: #2A3844; }"
        "QPushButton#start { background: #1E7A46; border-color: #2EAE68; }"
        "QPushButton#step { min-width: 36px; max-width: 36px; min-height: 30px; font-size: 14px; padding: 0; }"
        "QPushButton#padKey, QPushButton#padOk { min-height: 40px; font-size: 16px; }"
        "QPushButton#padOk { background: #1E7A46; border-color: #2EAE68; }"
        "QPushButton#estop { background: #C62828; border-color: #FF6B6B;"
        "  font-size: 16px; font-weight: 700; min-width: 84px; min-height: 48px; }"
        "QProgressBar { background: #0B1722; border: 1px solid #234056; border-radius: 3px; }"
        "QProgressBar::chunk { background: #FFE14A; }"));
}

void qt_demo::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placePad();
}

void qt_demo::placePad()
{
    if (!m_pad)
        return;
    m_pad->setGeometry(width() - 248 - 10, height() - 252 - 72, 248, 252);
}

void qt_demo::openPad(QLineEdit *edit)
{
    if (!edit || !edit->isEnabled() || !m_pad)
        return;
    if (padGestureBlocked(1))
        return;
    m_padEdit = edit;
    placePad();
    m_pad->show();
    m_pad->raise();
    edit->setFocus();
    edit->selectAll();
    markPadGesture(1);
    QTimer::singleShot(0, edit, [edit] {
        edit->setFocus();
        edit->selectAll();
    });
}

void qt_demo::hidePad()
{
    m_padEdit = nullptr;
    if (m_pad)
        m_pad->hide();
}

void qt_demo::dismissPad()
{
    if (!m_pad || !m_pad->isVisible() || padGestureBlocked(2))
        return;
    commitEdits();
    hidePad();
    markPadGesture(2);
}

bool qt_demo::padGestureBlocked(int gesture) const
{
    return m_padGesture != 0 && m_padGesture != gesture && m_padGuard.isValid() && m_padGuard.elapsed() < 80;
}

void qt_demo::markPadGesture(int gesture)
{
    m_padGesture = gesture;
    m_padGuard.restart();
}

void qt_demo::appendDigit(const QString &digit)
{
    if (!m_padEdit)
        return;
    if (m_padEdit->hasSelectedText() || m_padEdit->text().size() < m_padEdit->maxLength())
        m_padEdit->insert(digit);
}

void qt_demo::commitEdits()
{
    bool countOk = false;
    bool accelOk = false;
    bool intervalOk = false;
    const int count = m_countEdit->text().trimmed().toInt(&countOk);
    const int accel = m_accelEdit->text().trimmed().toInt(&accelOk);
    const int interval = m_intervalEdit->text().trimmed().toInt(&intervalOk);
    m_countValue = countOk ? qBound(kCountMin, count, kCountMax) : m_countValue;
    m_accelValue = accelOk ? qBound(kAccelMin, accel, kAccelMax) : m_accelValue;
    m_intervalValue = intervalOk ? qBound(kIntervalMin, interval, kIntervalMax) : m_intervalValue;
    m_countEdit->setText(QString::number(m_countValue));
    m_accelEdit->setText(QString::number(m_accelValue));
    m_intervalEdit->setText(QString::number(m_intervalValue));
    updateTestTime();
}

void qt_demo::applyMode(int index)
{
    if (index < 0 || index > 4 || !m_countEdit->isEnabled())
        return;
    m_countValue = kModeCounts[index];
    m_countEdit->setText(QString::number(m_countValue));
    updateTestTime();
}

void qt_demo::stepCount(int delta)
{
    if (!m_countEdit->isEnabled())
        return;
    bool ok = false;
    int value = m_countEdit->text().trimmed().toInt(&ok);
    if (!ok)
        value = m_countValue;
    m_countValue = qBound(kCountMin, value + delta, kCountMax);
    m_countEdit->setText(QString::number(m_countValue));
    updateTestTime();
}

void qt_demo::stepAccel(int delta)
{
    if (!m_accelEdit->isEnabled())
        return;
    bool ok = false;
    int value = m_accelEdit->text().trimmed().toInt(&ok);
    if (!ok)
        value = m_accelValue;
    m_accelValue = qBound(kAccelMin, value + delta, kAccelMax);
    m_accelEdit->setText(QString::number(m_accelValue));
}

void qt_demo::stepInterval(int delta)
{
    if (!m_intervalEdit->isEnabled())
        return;
    bool ok = false;
    int value = m_intervalEdit->text().trimmed().toInt(&ok);
    if (!ok)
        value = m_intervalValue;
    m_intervalValue = qBound(kIntervalMin, value + delta, kIntervalMax);
    m_intervalEdit->setText(QString::number(m_intervalValue));
    updateTestTime();
}

qint64 qt_demo::elapsedMs() const
{
    qint64 ms = m_elapsedMs;
    if (m_runClockActive)
        ms += m_runClock.elapsed();
    return ms;
}

void qt_demo::updateTestTime()
{
    if (!m_timeText)
        return;
    const int intervalMs = qMax(1, m_intervalValue) * 1000;
    qint64 remainingMs = 0;
    if (m_state == Idle) {
        remainingMs = static_cast<qint64>(m_countValue) * intervalMs;
    } else if (m_state == Pulsing || m_state == IntervalWait || m_state == Paused) {
        const int futureShocks = qMax(0, m_planned - m_done);
        int currentLeft = 0;
        if (m_state == Paused) {
            currentLeft = qMax(0, m_intervalLeftMs);
        } else if (m_state == IntervalWait && m_intervalTimer.isActive()) {
            currentLeft = qMax(0, m_intervalTimer.remainingTime());
        } else if (m_done >= m_planned) {
            const int step = 8;
            const int totalTicks = qMax(1, (m_replay.size() + step - 1) / step);
            const int doneTicks = m_replayIndex / step;
            currentLeft = qMax(0, totalTicks - doneTicks) * m_replayTimer.interval();
        } else {
            currentLeft = qMax(0, intervalMs - static_cast<int>(m_shockClock.elapsed()));
        }
        remainingMs = static_cast<qint64>(futureShocks) * intervalMs + currentLeft;
    }
    m_timeText->setText(QStringLiteral("%1 分钟").arg(remainingMs / 60000.0, 0, 'f', 2));
}

bool qt_demo::eventFilter(QObject *watched, QEvent *event)
{
    const bool press = event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin;
    if (press && m_pad) {
        QPoint global;
        if (event->type() == QEvent::MouseButtonPress) {
            auto *mouse = static_cast<QMouseEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            global = mouse->globalPosition().toPoint();
#else
            global = mouse->globalPos();
#endif
        } else {
            auto *touch = static_cast<QTouchEvent *>(event);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            if (!touch->points().isEmpty())
                global = touch->points().first().globalPosition().toPoint();
#else
            if (!touch->touchPoints().isEmpty())
                global = touch->touchPoints().first().screenPos().toPoint();
#endif
        }
        auto *widget = qobject_cast<QWidget *>(watched);
        if (widget && widget->window() == this) {
            const QRect padRect(m_pad->mapToGlobal(QPoint(0, 0)), m_pad->size());
            const bool onPad = m_pad->isVisible() && padRect.contains(global);
            QLineEdit *hitEdit = nullptr;
            QLineEdit *edits[] = {m_countEdit, m_accelEdit, m_intervalEdit};
            for (QLineEdit *candidate : edits) {
                if (!candidate || !candidate->isEnabled())
                    continue;
                const QRect box(candidate->mapToGlobal(QPoint(0, 0)), candidate->size());
                if (box.contains(global)) {
                    hitEdit = candidate;
                    break;
                }
            }
            if (hitEdit) {
                openPad(hitEdit);
                return true;
            }
            if (onPad)
                return false;
            if (m_pad->isVisible()) {
                dismissPad();
                return true;
            }
        }
    }

    auto *edit = qobject_cast<QLineEdit *>(watched);
    if (!edit || (edit != m_countEdit && edit != m_accelEdit && edit != m_intervalEdit))
        return QWidget::eventFilter(watched, event);

    if (event->type() == QEvent::MouseButtonPress && edit->isEnabled()) {
        openPad(edit);
        return true;
    }
    if (event->type() == QEvent::KeyPress && edit->isEnabled()) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Up || key->key() == Qt::Key_Down) {
            const int delta = key->key() == Qt::Key_Up ? 1 : -1;
            if (edit == m_countEdit)
                stepCount(delta);
            else if (edit == m_accelEdit)
                stepAccel(delta);
            else
                stepInterval(delta);
            return true;
        }
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            commitEdits();
            hidePad();
            return true;
        }
        if (key->key() == Qt::Key_Escape) {
            m_countEdit->setText(QString::number(m_countValue));
            m_accelEdit->setText(QString::number(m_accelValue));
            m_intervalEdit->setText(QString::number(m_intervalValue));
            hidePad();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void qt_demo::updateClock()
{
    m_clock->setText(QTime::currentTime().toString(QStringLiteral("hh:mm:ss")));
    if (m_state == Pulsing || m_state == IntervalWait)
        updateTestTime();
}

QString qt_demo::stateText() const
{
    switch (m_state) {
    case Idle: return QStringLiteral("待机");
    case Pulsing: return QStringLiteral("冲击中");
    case IntervalWait: return QStringLiteral("间隔等待");
    case Paused: return QStringLiteral("已暂停");
    case Finished: return QStringLiteral("完成");
    case Stopped: return QStringLiteral("已停止");
    case Estop: return QStringLiteral("急停");
    }
    return QString();
}

void qt_demo::setParamsEnabled(bool enabled)
{
    m_mode->setEnabled(enabled);
    m_countEdit->setEnabled(enabled);
    m_accelEdit->setEnabled(enabled);
    m_intervalEdit->setEnabled(enabled);
    m_countUp->setEnabled(enabled);
    m_countDown->setEnabled(enabled);
    m_accelUp->setEnabled(enabled);
    m_accelDown->setEnabled(enabled);
    m_intervalUp->setEnabled(enabled);
    m_intervalDown->setEnabled(enabled);
    if (!enabled)
        hidePad();
}

void qt_demo::setState(State state)
{
    m_state = state;
    m_status->setText(stateText());
    const bool running = state == Pulsing || state == IntervalWait || state == Paused;
    setParamsEnabled(!running && state != Estop);
    m_start->setEnabled(state == Idle || state == Finished || state == Stopped || state == Paused);
    m_start->setText(state == Paused ? QStringLiteral("继续") : QStringLiteral("开始"));
    m_pause->setEnabled(state == IntervalWait);
    m_stop->setEnabled(running);
    m_reset->setEnabled(!running);
    m_estop->setText(state == Estop ? QStringLiteral("已急停") : QStringLiteral("急停"));

    const char *color = "#8AA4BA";
    if (state == Pulsing || state == IntervalWait || state == Paused)
        color = "#FFE14A";
    else if (state == Finished)
        color = "#37D67A";
    else if (state == Estop || state == Stopped)
        color = "#FF5A5A";
    m_status->setStyleSheet(QStringLiteral("color: %1; font-size: 15px; font-weight: 700;").arg(QLatin1String(color)));
    updateTestTime();
}

void qt_demo::startTest()
{
    if (m_state != Idle && m_state != Finished && m_state != Stopped)
        return;

    commitEdits();
    hidePad();
    m_done = 0;
    m_planned = m_countValue;
    m_targetG = m_accelValue;
    m_measG = 0.0;
    m_intervalLeftMs = 0;
    m_elapsedMs = 0;
    m_runClock.restart();
    m_runClockActive = true;
    m_replay.clear();
    m_wave->clearCurve();
    m_countText->setText(QStringLiteral("0 / %1").arg(m_planned));
    m_peakText->setText(QStringLiteral("--"));
    m_progress->setRange(0, qMax(1, m_planned));
    m_progress->setValue(0);
    updateTestTime();
    beginPulse();
}

void qt_demo::beginPulse()
{
    if (m_state == Estop)
        return;

    m_shockClock.restart();
    const AirHammerTrace trace = synthesizeAirHammer(m_targetG);
    m_replay = trace.accelG;
    m_measG = trace.peakG;

    ++m_done;
    m_countText->setText(QStringLiteral("%1 / %2").arg(m_done).arg(m_planned));
    m_peakText->setText(QStringLiteral("%1 g").arg(m_measG, 0, 'f', 1));
    m_progress->setValue(m_done);

    m_wave->setCurve(m_replay, trace.durationMs, m_targetG);
    m_replayIndex = 0;
    m_replayTimer.start();
    setState(Pulsing);
}

void qt_demo::tickReplay()
{
    m_replayIndex += 8;
    if (m_replayIndex >= m_replay.size()) {
        m_wave->setShown(m_replay.size());
        finishReplay();
        return;
    }
    m_wave->setShown(m_replayIndex);
}

void qt_demo::finishReplay()
{
    m_replayTimer.stop();
    if (m_state == Estop || m_state == Stopped)
        return;
    if (m_done >= m_planned) {
        if (m_runClockActive) {
            m_elapsedMs += m_runClock.elapsed();
            m_runClockActive = false;
        }
        updateTestTime();
        setState(Finished);
        return;
    }
    const int periodMs = m_intervalValue * 1000;
    int remain = periodMs - static_cast<int>(m_shockClock.elapsed());
    if (remain < 50)
        remain = 50;
    m_intervalLeftMs = remain;
    m_intervalTimer.start(remain);
    setState(IntervalWait);
}

void qt_demo::resume()
{
    if (m_state != Paused)
        return;
    m_runClock.restart();
    m_runClockActive = true;
    const int waitMs = qMax(50, m_intervalLeftMs);
    m_intervalTimer.start(waitMs);
    setState(IntervalWait);
}

void qt_demo::stopTest(State endState)
{
    if (m_state == Idle && endState != Estop)
        return;
    if (m_state == Finished && endState == Stopped)
        return;
    m_intervalTimer.stop();
    m_replayTimer.stop();
    if (m_runClockActive) {
        m_elapsedMs += m_runClock.elapsed();
        m_runClockActive = false;
    }
    hidePad();
    updateTestTime();
    setState(endState);
}

void qt_demo::resetTest()
{
    if (m_state == Pulsing || m_state == IntervalWait || m_state == Paused)
        return;
    m_replayTimer.stop();
    m_intervalTimer.stop();
    hidePad();
    m_done = 0;
    m_planned = 0;
    m_elapsedMs = 0;
    m_runClockActive = false;
    m_replay.clear();
    m_wave->clearCurve();
    m_countText->setText(QStringLiteral("--"));
    m_peakText->setText(QStringLiteral("--"));
    m_progress->setRange(0, 1);
    m_progress->setValue(0);
    setState(Idle);
    updateTestTime();
}
