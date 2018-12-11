#include <QDebug>

#include "titlebar.h"
#include "ui_titlebar.h"
#include "debug_log.h"
#include <QPainter>
#include "setdialog.h"
#include "consoledialog.h"
#include "goopal.h"
#include "newsdialog.h"
#include "commondialog.h"
#include "datamgr.h"

TitleBar::TitleBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TitleBar)
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;

    ui->setupUi(this);    

    setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Background , QColor(229,229,229));
    setPalette(palette);

//    ui->transferBtn->setStyleSheet("QToolButton{background-image:url(pic2/transfer.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}");
    ui->newsBtn2->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/newsNum.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;color:white;}").arg(DataMgr::getDataMgr()->getWorkPath()));
    ui->newsBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/news.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()));
    ui->newsBtn->hide();
    ui->newsBtn2->hide();

    ui->minBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/minimize.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}"
                              "QToolButton:hover{background-image:url(%2pic2/minimize_hover.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()).arg(DataMgr::getDataMgr()->getWorkPath()));
    ui->closeBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/close.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}"
                                "QToolButton:hover{background-image:url(%2pic2/close_hover.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()).arg(DataMgr::getDataMgr()->getWorkPath()));

    ui->menuBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/setBtn.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()));

    ui->divLineLabel->setPixmap(QPixmap(DataMgr::getDataMgr()->getWorkPath() + "pic2/divLine.png"));
    ui->divLineLabel->setScaledContents(true);

   // connect( Goopal::getInstance(), SIGNAL(jsonDataUpdated(QString)), this, SLOT(jsonDataUpdated(QString)));

    timer = new QTimer(this);
    connect( timer, SIGNAL(timeout()), this, SLOT(onTimeOut()));
    timer->setInterval(10000);
    timer->start();

    DLOG_QT_WALLET_FUNCTION_END;
}

TitleBar::~TitleBar()
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;

    delete ui;

    DLOG_QT_WALLET_FUNCTION_END;
}

void TitleBar::on_minBtn_clicked()
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;

//    if( Goopal::getInstance()->minimizeToTray)
	if (false)
    {
        emit tray();
    }
    else
    {  
        emit minimum();
    }

    DLOG_QT_WALLET_FUNCTION_END;
}

void TitleBar::on_closeBtn_clicked()
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;

    //if( Goopal::getInstance()->closeToMinimize)
	if (false)
    {
        emit tray();
    }
    else
    {

		if (DataMgr::getInstance()->showPrivateTips) {
			CommonDialog commonDialog(CommonDialog::OkOnly);
			commonDialog.setText(tr("Keep your private key properly before closing Achain Wallet"));
			commonDialog.showTips();
			bool choice = commonDialog.pop();

			if(choice) {
				DataMgr::getInstance()->showPrivateTips = false;
				DataMgr::getInstance()->getSettings()->setValue("/settings/showPrivateTips", false);

			} else {
				DataMgr::getInstance()->showPrivateTips = true;
			}
		}

		CommonDialog commonDialog(CommonDialog::OkAndCancel);
		commonDialog.setText( tr( "Close Achain Wallet?"));
		bool choice = commonDialog.pop();

		if( choice) {
			emit closeGOP();
		} else {
			return;
		}
    }

    DLOG_QT_WALLET_FUNCTION_END;
}

void TitleBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setBrush(QColor(40,46,53));
    painter.setPen(QColor(40,46,53));
    painter.drawRect(QRect(0,0,132,37));
}


void TitleBar::on_menuBtn_clicked()
{
//    SetDialog* setDialog = new SetDialog;
//    Goopal::getInstance()->currentDialog = setDialog;
//    connect(setDialog,SIGNAL(settingSaved()),this,SLOT(saved()));
//    connect(setDialog,SIGNAL(destroyed()),this,SIGNAL(hideShadowWidget()));
//    emit showShadowWidget();
//    setDialog->setModal(true);
//    setDialog->setAttribute(Qt::WA_DeleteOnClose);
//    setDialog->show();

    SetDialog setDialog;
    connect(&setDialog,SIGNAL(settingSaved()),this,SLOT(saved()));
    connect(&setDialog,SIGNAL(resync()),this,SIGNAL(resync()));
    connect(&setDialog,SIGNAL(scan()),this,SIGNAL(scan()));
    setDialog.pop();

}

void TitleBar::saved()
{
    emit settingSaved();
}

void TitleBar::retranslator()
{
    ui->retranslateUi(this);
}

void TitleBar::on_newsBtn_clicked()
{
    if( ui->newsBtn2->text().toInt() == 0)
    {
        CommonDialog commonDialog(CommonDialog::OkOnly);
        commonDialog.setText( tr("No news!") );
        commonDialog.pop();
        return;
    }

    NewsDialog newsDialog;
    connect(&newsDialog, SIGNAL(showAccountPage(QString)), this, SIGNAL(showAccountPage(QString)));
//    emit showShadowWidget();
    newsDialog.pop();
//    emit hideShadowWidget();
}

void TitleBar::on_newsBtn2_clicked()
{
    on_newsBtn_clicked();
}

void TitleBar::on_consoleBtn_clicked()
{
//    ConsoleDialog* consoleDialog = new ConsoleDialog;
//    Goopal::getInstance()->currentDialog = consoleDialog;
//    connect(consoleDialog,SIGNAL(destroyed()),this,SIGNAL(hideShadowWidget()));
//    emit showShadowWidget();
//    consoleDialog->setModal(true);
//    consoleDialog->setAttribute(Qt::WA_DeleteOnClose);
//    consoleDialog->show();

    ConsoleDialog consoleDialog;
    consoleDialog.pop();

}
