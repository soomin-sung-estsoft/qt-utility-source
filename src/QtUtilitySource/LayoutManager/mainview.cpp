#include "mainview.h"
#include "widgeteventwrapper.h"

MainView::MainView(QWidget *parent)
    : QMainWindow(parent)
    , eventWrapper_(this)
    , layoutManager_(this)
{
    ui.setupUi(this);

    connect(&eventWrapper_, &WidgetEventWrapper::firstShown, this, &MainView::On_view_firstShown);
    connect(&layoutManager_, &LayoutManager::adjusted, this, &MainView::On_layoutManager_adjusted);
}

MainView::~MainView()
{}

void MainView::On_view_firstShown()
{
    // layoutmanager 초기화
    layoutManager_.Initialize();
}

void MainView::On_layoutManager_adjusted()
{
    // geometry가 아닌 요소들 조정

    qreal scalefactor = layoutManager_.ScaleFactor();

    QFont fontOk = ui.btnOK_->font();
    fontOk.setPixelSize(14 * scalefactor);
    
    ui.btnOK_->setFont(fontOk);


    QFont fontCancel = ui.btnCancel_->font();
    fontCancel.setPixelSize(14 * scalefactor);

    ui.btnCancel_->setFont(fontCancel);


    QFont fontInput = ui.txtInput_->font();
    fontInput.setPixelSize(14 * scalefactor);

    ui.txtInput_->setFont(fontInput);
}