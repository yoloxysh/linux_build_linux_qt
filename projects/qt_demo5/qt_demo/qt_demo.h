#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtCore/QVector>
#include <QtWidgets/QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QFrame;
class QProgressBar;
class QPushButton;
class WaveformView;

class qt_demo : public QWidget
{
    Q_OBJECT

public:
    explicit qt_demo(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum State {
        Idle,
        Pulsing,
        IntervalWait,
        Paused,
        Finished,
        Stopped,
        Estop
    };

    void buildUi();
    void buildPad();
    void applyStyle();
    void placePad();
    void openPad(QLineEdit *edit);
    void hidePad();
    void dismissPad();
    bool padGestureBlocked(int gesture) const;
    void markPadGesture(int gesture);
    void appendDigit(const QString &digit);
    void commitEdits();
    void applyMode(int index);
    void stepCount(int delta);
    void stepAccel(int delta);
    void stepInterval(int delta);
    void updateTestTime();
    qint64 elapsedMs() const;
    void setState(State state);
    void setParamsEnabled(bool enabled);
    void updateClock();
    void startTest();
    void beginPulse();
    void finishReplay();
    void tickReplay();
    void resume();
    void stopTest(State endState);
    void resetTest();
    QString stateText() const;

    WaveformView *m_wave = nullptr;
    QComboBox *m_mode = nullptr;
    QLineEdit *m_countEdit = nullptr;
    QLineEdit *m_accelEdit = nullptr;
    QLineEdit *m_intervalEdit = nullptr;
    QPushButton *m_countUp = nullptr;
    QPushButton *m_countDown = nullptr;
    QPushButton *m_accelUp = nullptr;
    QPushButton *m_accelDown = nullptr;
    QPushButton *m_intervalUp = nullptr;
    QPushButton *m_intervalDown = nullptr;
    QPushButton *m_start = nullptr;
    QPushButton *m_pause = nullptr;
    QPushButton *m_stop = nullptr;
    QPushButton *m_reset = nullptr;
    QPushButton *m_estop = nullptr;
    QFrame *m_pad = nullptr;
    QLineEdit *m_padEdit = nullptr;
    QElapsedTimer m_padGuard;
    int m_padGesture = 0;
    QLabel *m_status = nullptr;
    QLabel *m_clock = nullptr;
    QLabel *m_countText = nullptr;
    QLabel *m_peakText = nullptr;
    QLabel *m_timeText = nullptr;
    QProgressBar *m_progress = nullptr;

    QTimer m_clockTimer;
    QTimer m_replayTimer;
    QTimer m_intervalTimer;
    QElapsedTimer m_runClock;
    QElapsedTimer m_shockClock;

    State m_state = Idle;
    int m_done = 0;
    int m_planned = 0;
    int m_countValue = 100;
    int m_accelValue = 500;
    int m_intervalValue = 1;
    int m_replayIndex = 0;
    int m_intervalLeftMs = 0;
    qint64 m_elapsedMs = 0;
    bool m_runClockActive = false;
    double m_targetG = 500.0;
    double m_measG = 0.0;
    QVector<double> m_replay;
};
