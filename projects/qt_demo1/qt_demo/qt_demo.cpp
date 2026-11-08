#include "qt_demo.h"

#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QFrame>
#include <QtGui/QFontDatabase>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QtMath>

namespace {

QLabel *valueLabel(const QString &objectName)
{
    auto *label = new QLabel(QStringLiteral("--"));
    label->setObjectName(objectName);
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return label;
}

QLabel *caption(const QString &text)
{
    auto *label = new QLabel(text);
    label->setObjectName(QStringLiteral("caption"));
    return label;
}

} // namespace

qt_demo::qt_demo(QWidget *parent)
    : QWidget(parent)
{
    const QString fontFile = QDir(QCoreApplication::applicationDirPath())
                                 .filePath(QStringLiteral("wqy-microhei.ttc"));
    const int fontId = QFontDatabase::addApplicationFont(fontFile);
    if (fontId >= 0) {
        const QString family = QFontDatabase::applicationFontFamilies(fontId).value(0);
        if (!family.isEmpty()) {
            QFont font(family);
            font.setPointSize(12);
            setFont(font);
        }
    }

    buildUi();
    applyStyle();
    setRunning(false);
    setStatus(QStringLiteral("待机"), QStringLiteral("#8FB4D6"));

    m_tick.setInterval(40);
    connect(&m_tick, &QTimer::timeout, this, &qt_demo::onTick);
    connect(m_start, &QPushButton::clicked, this, &qt_demo::onStart);
    connect(m_pause, &QPushButton::clicked, this, &qt_demo::onPause);
    connect(m_stop, &QPushButton::clicked, this, &qt_demo::onStop);
    connect(m_reset, &QPushButton::clicked, this, &qt_demo::onReset);

    m_clock->setText(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd  hh:mm")));
}

qt_demo::~qt_demo() = default;

void qt_demo::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(8);

    auto *header = new QHBoxLayout;
    m_title = new QLabel(QStringLiteral("HMI  冲击试验台"));
    m_title->setObjectName(QStringLiteral("title"));
    m_clock = new QLabel;
    m_clock->setObjectName(QStringLiteral("clock"));
    m_status = new QLabel(QStringLiteral("待机"));
    m_status->setObjectName(QStringLiteral("status"));
    m_status->setAlignment(Qt::AlignCenter);
    header->addWidget(m_title);
    header->addStretch(1);
    header->addWidget(m_clock);
    header->addSpacing(12);
    header->addWidget(m_status);
    root->addLayout(header);

    auto *body = new QHBoxLayout;
    body->setSpacing(8);

    auto *left = new QVBoxLayout;
    left->setSpacing(8);

    auto *setup = new QGroupBox(QStringLiteral("试验设定"));
    auto *grid = new QGridLayout(setup);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);

    m_count = new QSpinBox;
    m_count->setRange(1, 999);
    m_count->setValue(10);
    m_count->setSuffix(QStringLiteral(" 次"));

    m_accel = new QDoubleSpinBox;
    m_accel->setRange(5.0, 200.0);
    m_accel->setDecimals(1);
    m_accel->setSingleStep(1.0);
    m_accel->setValue(50.0);
    m_accel->setSuffix(QStringLiteral(" g"));

    m_pulse = new QSpinBox;
    m_pulse->setRange(2, 30);
    m_pulse->setValue(11);
    m_pulse->setSuffix(QStringLiteral(" ms"));

    m_tol = new QDoubleSpinBox;
    m_tol->setRange(1.0, 20.0);
    m_tol->setDecimals(1);
    m_tol->setValue(10.0);
    m_tol->setSuffix(QStringLiteral(" %"));

    m_mode = new QComboBox;
    m_mode->addItem(QStringLiteral("半正弦"), 0);
    m_mode->addItem(QStringLiteral("后峰锯齿"), 1);
    m_mode->addItem(QStringLiteral("梯形"), 2);

    grid->addWidget(caption(QStringLiteral("测试次数")), 0, 0);
    grid->addWidget(m_count, 0, 1);
    grid->addWidget(caption(QStringLiteral("冲击加速度")), 1, 0);
    grid->addWidget(m_accel, 1, 1);
    grid->addWidget(caption(QStringLiteral("脉冲宽度")), 2, 0);
    grid->addWidget(m_pulse, 2, 1);
    grid->addWidget(caption(QStringLiteral("允许偏差")), 3, 0);
    grid->addWidget(m_tol, 3, 1);
    grid->addWidget(caption(QStringLiteral("测试模式")), 4, 0);
    grid->addWidget(m_mode, 4, 1);
    left->addWidget(setup);

    auto *live = new QGroupBox(QStringLiteral("实时测量"));
    auto *liveGrid = new QGridLayout(live);
    liveGrid->setVerticalSpacing(4);
    m_accelValue = valueLabel(QStringLiteral("bigValue"));
    m_peakValue = valueLabel(QStringLiteral("midValue"));
    m_countValue = valueLabel(QStringLiteral("midValue"));
    m_pulseValue = valueLabel(QStringLiteral("midValue"));
    m_accelValue->setText(QStringLiteral("0.0 g"));
    m_peakValue->setText(QStringLiteral("0.0 g"));
    m_countValue->setText(QStringLiteral("0 / 0"));
    m_pulseValue->setText(QStringLiteral("0 ms"));
    liveGrid->addWidget(caption(QStringLiteral("当前加速度")), 0, 0);
    liveGrid->addWidget(m_accelValue, 0, 1);
    liveGrid->addWidget(caption(QStringLiteral("本锤峰值")), 1, 0);
    liveGrid->addWidget(m_peakValue, 1, 1);
    liveGrid->addWidget(caption(QStringLiteral("已完成 / 计划")), 2, 0);
    liveGrid->addWidget(m_countValue, 2, 1);
    liveGrid->addWidget(caption(QStringLiteral("本锤脉宽")), 3, 0);
    liveGrid->addWidget(m_pulseValue, 3, 1);
    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setFormat(QStringLiteral("进度  %p%"));
    m_progress->setTextVisible(true);
    liveGrid->addWidget(m_progress, 4, 0, 1, 2);
    left->addWidget(live);

    m_verdict = new QLabel(QStringLiteral("判定  —"));
    m_verdict->setObjectName(QStringLiteral("verdict"));
    m_verdict->setAlignment(Qt::AlignCenter);
    left->addWidget(m_verdict);
    left->addStretch(1);

    auto *right = new QVBoxLayout;
    right->setSpacing(8);
    auto *logBox = new QGroupBox(QStringLiteral("冲击记录"));
    auto *logLayout = new QVBoxLayout(logBox);
    m_log = new QTableWidget(0, 5);
    m_log->setHorizontalHeaderLabels({
        QStringLiteral("#"),
        QStringLiteral("峰值 g"),
        QStringLiteral("脉宽"),
        QStringLiteral("判定"),
        QStringLiteral("说明")
    });
    m_log->verticalHeader()->setVisible(false);
    m_log->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_log->setSelectionMode(QAbstractItemView::NoSelection);
    m_log->setFocusPolicy(Qt::NoFocus);
    m_log->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_log->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_log->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_log->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_log->horizontalHeader()->setStretchLastSection(true);
    m_log->setShowGrid(false);
    logLayout->addWidget(m_log);
    right->addWidget(logBox, 1);

    auto *keys = new QHBoxLayout;
    keys->setSpacing(8);
    m_start = new QPushButton(QStringLiteral("启动"));
    m_pause = new QPushButton(QStringLiteral("暂停"));
    m_stop = new QPushButton(QStringLiteral("急停"));
    m_reset = new QPushButton(QStringLiteral("复位"));
    m_start->setObjectName(QStringLiteral("start"));
    m_pause->setObjectName(QStringLiteral("pause"));
    m_stop->setObjectName(QStringLiteral("stop"));
    m_reset->setObjectName(QStringLiteral("reset"));
    for (auto *button : {m_start, m_pause, m_stop, m_reset}) {
        button->setMinimumHeight(52);
        button->setCursor(Qt::PointingHandCursor);
        keys->addWidget(button);
    }
    right->addLayout(keys);

    body->addLayout(left, 4);
    body->addLayout(right, 6);
    root->addLayout(body, 1);

    m_hint = new QLabel(QStringLiteral("台架联机：模拟通道    采样 1 kHz    波形按所选模式生成，峰值落入允许偏差即为合格。"));
    m_hint->setWordWrap(true);
    m_hint->setObjectName(QStringLiteral("hint"));
    root->addWidget(m_hint);
}

void qt_demo::applyStyle()
{
    setStyleSheet(QStringLiteral(
        "QWidget { background: #0B1F3A; color: #E7F0FA; }"
        "QLabel#title { font-size: 22px; font-weight: 700; color: #FFE600; background: transparent; }"
        "QLabel#clock, QLabel#hint, QLabel#caption { color: #8FB4D6; background: transparent; font-size: 12px; }"
        "QLabel#status { min-width: 88px; padding: 4px 10px; border-radius: 4px; background: #16365C; color: #8FB4D6; font-weight: 700; }"
        "QLabel#bigValue { font-size: 28px; font-weight: 700; color: #FFE600; background: transparent; }"
        "QLabel#midValue { font-size: 16px; font-weight: 700; color: #FFFFFF; background: transparent; }"
        "QLabel#verdict { font-size: 16px; font-weight: 700; padding: 8px; border-radius: 4px; background: #16365C; color: #8FB4D6; }"
        "QGroupBox { border: 1px solid #1E4A78; border-radius: 6px; margin-top: 10px; padding: 8px; font-weight: 700; color: #B9D4EE; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
        "QSpinBox, QDoubleSpinBox, QComboBox { background: #102844; border: 1px solid #2A5A8A; border-radius: 4px; padding: 4px 8px; min-height: 28px; color: #FFFFFF; }"
        "QComboBox QAbstractItemView { background: #102844; color: #FFFFFF; selection-background-color: #1E4A78; }"
        "QProgressBar { border: 1px solid #2A5A8A; border-radius: 4px; background: #102844; text-align: center; color: #E7F0FA; min-height: 18px; }"
        "QProgressBar::chunk { background: #2E8B57; border-radius: 3px; }"
        "QTableWidget { background: #102844; alternate-background-color: #0E223C; border: none; gridline-color: #1E4A78; }"
        "QHeaderView::section { background: #16365C; color: #B9D4EE; border: none; padding: 4px; }"
        "QPushButton { border: none; border-radius: 6px; font-size: 16px; font-weight: 700; color: #FFFFFF; }"
        "QPushButton#start { background: #1F8A4C; }"
        "QPushButton#start:disabled { background: #24523A; color: #8FB4D6; }"
        "QPushButton#pause { background: #C47B12; }"
        "QPushButton#pause:disabled { background: #5C4A28; color: #8FB4D6; }"
        "QPushButton#stop { background: #C0392B; }"
        "QPushButton#stop:disabled { background: #5C3030; color: #8FB4D6; }"
        "QPushButton#reset { background: #1E4A78; }"
        "QPushButton#reset:disabled { background: #16365C; color: #6E8EAE; }"
    ));
}

double qt_demo::targetG() const
{
    return m_accel->value();
}

int qt_demo::pulseMs() const
{
    return m_pulse->value();
}

double qt_demo::tolerancePct() const
{
    return m_tol->value();
}

QString qt_demo::modeName() const
{
    return m_mode->currentText();
}

void qt_demo::setStatus(const QString &text, const QString &color)
{
    m_status->setText(text);
    m_status->setStyleSheet(QStringLiteral(
        "QLabel#status { min-width: 88px; padding: 4px 10px; border-radius: 4px; "
        "background: #16365C; font-weight: 700; color: %1; }").arg(color));
}

void qt_demo::setRunning(bool running)
{
    m_count->setEnabled(!running);
    m_accel->setEnabled(!running);
    m_pulse->setEnabled(!running);
    m_tol->setEnabled(!running);
    m_mode->setEnabled(!running);
    m_start->setEnabled(!running && m_phase != Finished);
    m_pause->setEnabled(running);
    m_stop->setEnabled(running || m_paused);
    m_reset->setEnabled(!running);
}

void qt_demo::onStart()
{
    if (m_phase == Finished || m_phase == Aborted) {
        onReset();
    }
    if (m_paused) {
        m_paused = false;
        m_tick.start();
        setRunning(true);
        setStatus(QStringLiteral("运行"), QStringLiteral("#3DDC97"));
        m_pause->setText(QStringLiteral("暂停"));
        return;
    }
    m_planned = m_count->value();
    m_done = 0;
    m_passCount = 0;
    m_failCount = 0;
    m_shots.clear();
    m_log->setRowCount(0);
    m_verdict->setText(QStringLiteral("判定  试验中"));
    m_verdict->setStyleSheet(QStringLiteral(
        "QLabel#verdict { font-size: 16px; font-weight: 700; padding: 8px; border-radius: 4px; "
        "background: #16365C; color: #FFE600; }"));
    beginShot();
    m_tick.start();
    setRunning(true);
    setStatus(QStringLiteral("运行"), QStringLiteral("#3DDC97"));
}

void qt_demo::onPause()
{
    if (!m_tick.isActive()) {
        return;
    }
    m_tick.stop();
    m_paused = true;
    m_start->setEnabled(true);
    m_start->setText(QStringLiteral("继续"));
    m_pause->setEnabled(false);
    setStatus(QStringLiteral("暂停"), QStringLiteral("#F0B429"));
}

void qt_demo::onStop()
{
    m_tick.stop();
    m_paused = false;
    m_g = 0.0;
    m_phase = Aborted;
    m_start->setText(QStringLiteral("启动"));
    setRunning(false);
    m_start->setEnabled(false);
    updateReadout();
    setStatus(QStringLiteral("急停"), QStringLiteral("#FF5A4E"));
    m_verdict->setText(QStringLiteral("判定  急停中止  合格 %1  不合格 %2")
                           .arg(m_passCount)
                           .arg(m_failCount));
    m_verdict->setStyleSheet(QStringLiteral(
        "QLabel#verdict { font-size: 16px; font-weight: 700; padding: 8px; border-radius: 4px; "
        "background: #4A1C1C; color: #FF8A80; }"));
    m_hint->setText(QStringLiteral("急停已切断冲击输出。复位后才能重新设定并启动。"));
}

void qt_demo::onReset()
{
    m_tick.stop();
    m_paused = false;
    m_phase = Idle;
    m_done = 0;
    m_planned = 0;
    m_passCount = 0;
    m_failCount = 0;
    m_g = 0.0;
    m_peak = 0.0;
    m_shotElapsedMs = 0;
    m_shots.clear();
    m_log->setRowCount(0);
    m_progress->setValue(0);
    m_start->setText(QStringLiteral("启动"));
    m_pause->setText(QStringLiteral("暂停"));
    m_verdict->setText(QStringLiteral("判定  —"));
    m_verdict->setStyleSheet(QStringLiteral(
        "QLabel#verdict { font-size: 16px; font-weight: 700; padding: 8px; border-radius: 4px; "
        "background: #16365C; color: #8FB4D6; }"));
    m_hint->setText(QStringLiteral("台架联机：模拟通道    采样 1 kHz    波形按所选模式生成，峰值落入允许偏差即为合格。"));
    setRunning(false);
    updateReadout();
    setStatus(QStringLiteral("待机"), QStringLiteral("#8FB4D6"));
}

void qt_demo::beginShot()
{
    m_phase = Arming;
    m_shotElapsedMs = 0;
    m_phaseBudgetMs = 280;
    m_peak = 0.0;
    m_g = 0.0;
    m_shotTimer.restart();
}

void qt_demo::finishShot(bool pass, const QString &note)
{
    Shot shot;
    shot.index = m_done + 1;
    shot.peakG = m_peak;
    shot.pulseMs = pulseMs();
    shot.pass = pass;
    shot.note = note;
    m_shots.append(shot);
    appendLog(shot);
    if (pass) {
        ++m_passCount;
    } else {
        ++m_failCount;
    }
    ++m_done;
    m_g = 0.0;
    if (m_done >= m_planned) {
        m_phase = Finished;
        m_tick.stop();
        m_start->setText(QStringLiteral("启动"));
        setRunning(false);
        m_start->setEnabled(false);
        const bool allPass = m_failCount == 0;
        setStatus(allPass ? QStringLiteral("完成") : QStringLiteral("完成"),
                  allPass ? QStringLiteral("#3DDC97") : QStringLiteral("#F0B429"));
        m_verdict->setText(QStringLiteral("判定  %1    合格 %2 / %3")
                               .arg(allPass ? QStringLiteral("合格") : QStringLiteral("不合格"))
                               .arg(m_passCount)
                               .arg(m_planned));
        m_verdict->setStyleSheet(QStringLiteral(
            "QLabel#verdict { font-size: 16px; font-weight: 700; padding: 8px; border-radius: 4px; "
            "background: %1; color: %2; }")
                                     .arg(allPass ? QStringLiteral("#143D28") : QStringLiteral("#4A3010"),
                                          allPass ? QStringLiteral("#3DDC97") : QStringLiteral("#F0B429")));
        m_hint->setText(QStringLiteral("试验结束。模式 %1，目标 %2 g，脉宽 %3 ms，偏差 ±%4%。复位后可改参数再测。")
                            .arg(modeName())
                            .arg(targetG(), 0, 'f', 1)
                            .arg(pulseMs())
                            .arg(tolerancePct(), 0, 'f', 1));
    } else {
        m_phase = Dwell;
        m_shotElapsedMs = 0;
        m_phaseBudgetMs = 360;
    }
    updateReadout();
}

void qt_demo::appendLog(const Shot &shot)
{
    const int row = m_log->rowCount();
    m_log->insertRow(row);
    const QString peak = QString::number(shot.peakG, 'f', 1);
    auto *index = new QTableWidgetItem(QString::number(shot.index));
    auto *peakItem = new QTableWidgetItem(peak);
    auto *width = new QTableWidgetItem(QStringLiteral("%1 ms").arg(shot.pulseMs));
    auto *verdict = new QTableWidgetItem(shot.pass ? QStringLiteral("合格") : QStringLiteral("超差"));
    auto *note = new QTableWidgetItem(shot.note);
    verdict->setForeground(shot.pass ? QColor(QStringLiteral("#3DDC97"))
                                     : QColor(QStringLiteral("#FF8A80")));
    for (auto *item : {index, peakItem, width, verdict, note}) {
        item->setTextAlignment(Qt::AlignCenter);
        item->setFlags(Qt::ItemIsEnabled);
    }
    note->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_log->setItem(row, 0, index);
    m_log->setItem(row, 1, peakItem);
    m_log->setItem(row, 2, width);
    m_log->setItem(row, 3, verdict);
    m_log->setItem(row, 4, note);
    m_log->scrollToBottom();
}

void qt_demo::updateReadout()
{
    m_accelValue->setText(QStringLiteral("%1 g").arg(m_g, 0, 'f', 1));
    m_peakValue->setText(QStringLiteral("%1 g").arg(m_peak, 0, 'f', 1));
    m_countValue->setText(QStringLiteral("%1 / %2").arg(m_done).arg(m_planned));
    m_pulseValue->setText(QStringLiteral("%1 ms").arg(m_phase == Idle ? 0 : pulseMs()));
    const int pct = m_planned > 0 ? qBound(0, m_done * 100 / m_planned, 100) : 0;
    m_progress->setValue(pct);
    m_clock->setText(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd  hh:mm:ss")));
}

void qt_demo::onTick()
{
    const int step = m_tick.interval();
    m_shotElapsedMs += step;

    if (m_phase == Arming) {
        m_g = 0.4 + 0.2 * qSin(m_shotElapsedMs / 40.0);
        if (m_shotElapsedMs >= m_phaseBudgetMs) {
            m_phase = Rising;
            m_shotElapsedMs = 0;
            m_phaseBudgetMs = qMax(80, pulseMs() * 18);
        }
    } else if (m_phase == Rising || m_phase == Peak || m_phase == Falling) {
        const double width = m_phaseBudgetMs;
        const double t = qBound(0.0, m_shotElapsedMs / width, 1.0);
        const int mode = m_mode->currentData().toInt();
        double shape = 0.0;
        if (mode == 0) {
            shape = qSin(M_PI * t);
        } else if (mode == 1) {
            shape = t;
        } else {
            shape = t < 0.15 ? t / 0.15 : (t > 0.85 ? (1.0 - t) / 0.15 : 1.0);
        }
        /* Small repeatable scatter so a tight tolerance can fail a shot. */
        const double scatter = 1.0 + 0.06 * qSin(m_done * 1.7 + 0.4);
        const double aimed = targetG() * scatter;
        m_g = qMax(0.0, aimed * shape);
        m_peak = qMax(m_peak, m_g);
        if (t >= 0.45 && t < 0.7) {
            m_phase = Peak;
        } else if (t >= 0.7) {
            m_phase = Falling;
        }
        if (m_shotElapsedMs >= m_phaseBudgetMs) {
            const double low = targetG() * (1.0 - tolerancePct() / 100.0);
            const double high = targetG() * (1.0 + tolerancePct() / 100.0);
            const bool pass = m_peak >= low && m_peak <= high;
            const QString note = pass
                ? QStringLiteral("%1  落入 ±%2%")
                      .arg(modeName())
                      .arg(tolerancePct(), 0, 'f', 1)
                : QStringLiteral("%1  峰值超出 %2 ~ %3 g")
                      .arg(modeName())
                      .arg(low, 0, 'f', 1)
                      .arg(high, 0, 'f', 1);
            finishShot(pass, note);
        }
    } else if (m_phase == Dwell) {
        m_g = qMax(0.0, m_g * 0.6);
        if (m_shotElapsedMs >= m_phaseBudgetMs) {
            beginShot();
        }
    }

    updateReadout();
}
