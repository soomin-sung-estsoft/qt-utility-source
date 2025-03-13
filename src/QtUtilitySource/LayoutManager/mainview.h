#pragma once

#include "ui_mainview.h"
#include "widgeteventwrapper.h"
#include "layoutmanager.h"

#include <QMainWindow>

class MainView : public QMainWindow
{
    Q_OBJECT

public:
    MainView(QWidget *parent = nullptr);
    ~MainView();

private slots:
    void On_view_firstShown();
    void On_layoutManager_adjusted();

private:
    Ui::MainViewClass ui;
    WidgetEventWrapper eventWrapper_;
    LayoutManager layoutManager_;
};
