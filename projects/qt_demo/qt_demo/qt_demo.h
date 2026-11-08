#pragma once

#include <QtWidgets/QWidget>
#include "ui_qt_demo.h"

class qt_demo : public QWidget
{
    Q_OBJECT

public:
    qt_demo(QWidget *parent = nullptr);
    ~qt_demo();

private:
    Ui::qt_demoClass ui;
};

