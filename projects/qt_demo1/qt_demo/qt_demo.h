#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtCore/QVector>

class QLabel;
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QProgressBar;
class QTableWidget;

class qt_demo : public QWidget
{
    Q_OBJECT

public:
    explicit qt_demo(QWidget *parent = nullptr);
    ~qt_demo() override;

private slots:
    void onStart();
    void onPause();
    void onStop();
    void onReset();
    void onTick();

private:
    enum Phase {
        Idle,
        Arming,
        Rising,
        Peak,
        Falling,
        Dwell,
        Finished,
        Aborted
    };

    struct Shot {
        int index;
        double peakG;
        int pulseMs;
        bool pass;
        QString note;
    };

    void buildUi();
    void applyStyle();
    void setRunning(bool running);
    void beginShot();
    void finishShot(bool pass, const QString &note);
    void updateReadout();
    void appendLog(const Shot &shot);
    void setStatus(const QString &text, const QString &color);
    double targetG() const;
    int pulseMs() const;
    double tolerancePct() const;
    QString modeName() const;

    QLabel *m_title = nullptr;
    QLabel *m_clock = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_accelValue = nullptr;
    QLabel *m_peakValue = nullptr;
    QLabel *m_countValue = nullptr;
    QLabel *m_pulseValue = nullptr;
    QLabel *m_verdict = nullptr;
    QLabel *m_hint = nullptr;

    QSpinBox *m_count = nullptr;
    QDoubleSpinBox *m_accel = nullptr;
    QSpinBox *m_pulse = nullptr;
    QDoubleSpinBox *m_tol = nullptr;
    QComboBox *m_mode = nullptr;

    QProgressBar *m_progress = nullptr;
    QTableWidget *m_log = nullptr;

    QPushButton *m_start = nullptr;
    QPushButton *m_pause = nullptr;
    QPushButton *m_stop = nullptr;
    QPushButton *m_reset = nullptr;

    QTimer m_tick;
    QElapsedTimer m_shotTimer;
    QVector<Shot> m_shots;

    Phase m_phase = Idle;
    bool m_paused = false;
    int m_done = 0;
    int m_planned = 0;
    int m_passCount = 0;
    int m_failCount = 0;
    double m_g = 0.0;
    double m_peak = 0.0;
    int m_shotElapsedMs = 0;
    int m_phaseBudgetMs = 0;
};
