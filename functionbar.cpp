#include "functionbar.h"
#include "ui_functionbar.h"
#include "goopal.h"
#include "commondialog.h"
#include "debug_log.h"
#include "datamgr.h"
#include "extra/dynamicmove.h"

#include <QPainter>
#include <QDebug>


FunctionBar::FunctionBar(QWidget *parent) :
QWidget(parent),
ui(new Ui::FunctionBar),wait_for_update(0)
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;
	ui->setupUi(this);
	setAutoFillBackground(true);
	QPalette palette;
	palette.setColor(QPalette::Background, QColor(40, 46, 53));
	setPalette(palette);
	ui->activeLabel->setPixmap(QPixmap(DataMgr::getDataMgr()->getWorkPath() + "pic2/active.png"));
	ui->activeLabel->setScaledContents(true);
	ui->logoLabel->setPixmap(QPixmap(DataMgr::getDataMgr()->getWorkPath() + "pic2/logo.png"));
	ui->logoLabel->setScaledContents(true);

	choosePage(1);
	ui->contactBtn->hide();
	QPalette palette1;
	palette1.setBrush(QPalette::Text, QBrush(Qt::white));
	palette1.setBrush(QPalette::Window, QBrush(Qt::white));
    ui->assertComboBox->setPalette(palette1);

    //增加下拉框图片
#ifdef WIN32
    QPixmap icon1(DataMgr::getDataMgr()->getWorkPath() + "pic2/arrowdown.png");
    ui->assertComboBox->setIconSize(QSize(30, 20));
    ui->assertComboBox->setWindowIcon(icon1);
#endif // WIN32//zxlrun
	ui->versionLabel_2->setText("Achain Wallet\r\n" ACHAIN_WALLET_VERSION_STR);

	for (auto currencyInfo : DataMgr::getInstance()->getCurrencyList())
	{
		ui->assertComboBox->addItem(currencyInfo.name, QVariant(currencyInfo.id));
	}

    ui->assertComboBox->setStyleSheet(
                QString("QComboBox {"
                    "background-color: rgba(47, 47, 53, 10);"
                    "background-repeat: no-repeat; "
                    "selection-background-color: rgb(85, 85, 255); "
                    "selection-color: rgb(255, 255, 255); "
                "}"
                "QComboBox::drop-down {"
                        "background-color: rgba(47, 47, 53, 10);"
                        "height:30px;"
                "}"
                "QComboBox::down-arrow { image: url(%1pic2/assetComboxArrow.png);}").arg(DataMgr::getDataMgr()->getWorkPath())
              );
/*    ui->assertComboBox->setStyleSheet(
        //"QComboBox::drop-down{subcontrol-origin:padding; subcontrol-position:top right; width:16px; border-left-width:1px;border-left-color:darkgray; border-left-style:solid; border-top-right-radius:3px; border-bottom-right-radius:3px;}"
        //"QComboBox QAbstractItemView{border: 2px solid #4E6D8C;}"
        "QComboBox::down-arrow {image: url(pic2/arrowdown.png);}"
    );*/
    connect(this, &FunctionBar::assetTypeChange, DataMgr::getInstance(), &DataMgr::onAssetChange);
    connect(DataMgr::getInstance(), &DataMgr::onGetInfo, this, &FunctionBar::getInfo);
    connect(DataMgr::getInstance(), &DataMgr::assetTypeGet, this, &FunctionBar::setAssetInfo);

    updated = false;
    scan = false;
    DLOG_QT_WALLET_FUNCTION_END;
}

FunctionBar::~FunctionBar()
{
    delete ui;
}

void FunctionBar::setAssetInfo()
{
	ui->assertComboBox->clear();

	for (auto currencyInfo : DataMgr::getInstance()->getCurrencyList())
	{
		ui->assertComboBox->addItem(currencyInfo.name, QVariant(currencyInfo.id));
	}
}

void FunctionBar::on_surveyBtn_clicked()
{
    choosePage(1);
    emit showMainPage();
}

void FunctionBar::on_assertComboBox_currentIndexChanged(int index)
{
	int currencyId = ui->assertComboBox->itemData(index).toInt();
	const CurrencyInfo& currencyInfo = DataMgr::getInstance()->getCurrencyById(currencyId);

	if (currencyInfo.id != DataMgr::getCurrentCurrency().id)
	{
		DataMgr::getInstance()->setCurrentCurrency(currencyInfo);
		emit assetTypeChange();
	}
}

void FunctionBar::on_accountBtn_clicked()
{
    mutexForAddressMap.lock();
    int size = DataMgr::getInstance()->getAccountInfo()->size();
    mutexForAddressMap.unlock();

    if( size != 0)   {
        // 有至少一个账户
        choosePage(2);
		const QString& accountName = DataMgr::getInstance()->getCurrentAccount();
        emit showAccountPage(accountName);
    } else {
        CommonDialog commonDialog(CommonDialog::OkOnly);
        commonDialog.setText(tr("You have no accounts,\nadd an account first"));
        commonDialog.pop();
        on_surveyBtn_clicked();
    }
}

void FunctionBar::on_transferBtn_clicked()
{
    mutexForAddressMap.lock();
    int size = DataMgr::getInstance()->getAccountInfo()->size();
    mutexForAddressMap.unlock();
    if( size != 0 && updated)   {
        // 有至少一个账户
        choosePage(3);
        emit showTransferPage();

	} else if (updated == false) {
		CommonDialog commonDialog(CommonDialog::OkOnly);
		const QString& content = tr("Please wait for wallet to sync");
		if (wait_for_update > 0)
			commonDialog.setText(content + "(" + QString::number(wait_for_update) + " " + tr("blocks)"));
		else
			commonDialog.setText(content);

        commonDialog.pop();
        on_surveyBtn_clicked();

	} else {
        CommonDialog commonDialog(CommonDialog::OkOnly);
        commonDialog.setText(tr("No account for transfering,\nadd an account first"));
        commonDialog.pop();
        on_surveyBtn_clicked();
    }
}

void FunctionBar::on_delegateBtn_clicked()
{
//	choosePage(4);
//   emit showDelegatePage();

	CommonDialog commonDialog(CommonDialog::OkOnly);
	commonDialog.setText(tr("Limited to developer accounts.     "));
	commonDialog.pop();
    on_surveyBtn_clicked();
}

void FunctionBar::on_contactBtn_clicked()
{
    choosePage(5);
    emit showContactPage();
}

void FunctionBar::choosePage(int pageIndex)
{
//    ui->activeLabel->move(0, 17 + 94 * ( pageIndex - 1));
    DynamicMove* dynamicMove = new DynamicMove( ui->activeLabel, QPoint(0,17 + 94 * ( pageIndex - 1)), 20, 10, this);
    dynamicMove->start();

    switch (pageIndex) {
    case 1:
        ui->surveyBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/surveyBtn.png"));
        ui->surveyBtn->setIconSize(QSize(29,28));
        ui->surveyBtn->setStyleSheet("background:transparent;color:white");
        ui->surveyBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->accountBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/accountBtn_unselected.png"));
        ui->accountBtn->setIconSize(QSize(24,17));
        ui->accountBtn->setStyleSheet("background:transparent;color:grey");
        ui->accountBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->delegateBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/delegateBtn_unselected.png"));
        ui->delegateBtn->setIconSize(QSize(26,26));
        ui->delegateBtn->setStyleSheet("background:transparent;color:grey");
        ui->delegateBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->transferBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/transferBtn_unselected.png"));
        ui->transferBtn->setIconSize(QSize(23,18));
        ui->transferBtn->setStyleSheet("background:transparent;color:grey");
        ui->transferBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->contactBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/contactBtn_unselected.png"));
        ui->contactBtn->setIconSize(QSize(23,22));
        ui->contactBtn->setStyleSheet("background:transparent;color:grey");
        ui->contactBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        break;

    case 2:
        ui->surveyBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/surveyBtn_unselected.png"));
        ui->surveyBtn->setIconSize(QSize(29,28));
        ui->surveyBtn->setStyleSheet("background:transparent;color:grey");
        ui->surveyBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->accountBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/accountBtn.png"));
        ui->accountBtn->setIconSize(QSize(24,17));
        ui->accountBtn->setStyleSheet("background:transparent;color:white");
        ui->accountBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->delegateBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/delegateBtn_unselected.png"));
        ui->delegateBtn->setIconSize(QSize(26,26));
        ui->delegateBtn->setStyleSheet("background:transparent;color:grey");
        ui->delegateBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->transferBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/transferBtn_unselected.png"));
        ui->transferBtn->setIconSize(QSize(23,18));
        ui->transferBtn->setStyleSheet("background:transparent;color:grey");
        ui->transferBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->contactBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/contactBtn_unselected.png"));
        ui->contactBtn->setIconSize(QSize(23,22));
        ui->contactBtn->setStyleSheet("background:transparent;color:grey");
        ui->contactBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        break;

    case 3:
        ui->surveyBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/surveyBtn_unselected.png"));
        ui->surveyBtn->setIconSize(QSize(29,28));
        ui->surveyBtn->setStyleSheet("background:transparent;color:grey");
        ui->surveyBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->accountBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/accountBtn_unselected.png"));
        ui->accountBtn->setIconSize(QSize(24,17));
        ui->accountBtn->setStyleSheet("background:transparent;color:grey");
        ui->accountBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->transferBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/transferBtn.png"));
        ui->transferBtn->setIconSize(QSize(23,18));
        ui->transferBtn->setStyleSheet("background:transparent;color:white");
        ui->transferBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->delegateBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/delegateBtn_unselected.png"));
        ui->delegateBtn->setIconSize(QSize(26,26));
        ui->delegateBtn->setStyleSheet("background:transparent;color:grey");
        ui->delegateBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->contactBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/contactBtn_unselected.png"));
        ui->contactBtn->setIconSize(QSize(23,22));
        ui->contactBtn->setStyleSheet("background:transparent;color:grey");
        ui->contactBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        break;

    case 4:
        ui->surveyBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/surveyBtn_unselected.png"));
        ui->surveyBtn->setIconSize(QSize(29,28));
        ui->surveyBtn->setStyleSheet("background:transparent;color:grey");
        ui->surveyBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->accountBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/accountBtn_unselected.png"));
        ui->accountBtn->setIconSize(QSize(24,17));
        ui->accountBtn->setStyleSheet("background:transparent;color:grey");
        ui->accountBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->transferBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/transferBtn_unselected.png"));
        ui->transferBtn->setIconSize(QSize(23,18));
        ui->transferBtn->setStyleSheet("background:transparent;color:grey");
        ui->transferBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->delegateBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/delegateBtn.png"));
        ui->delegateBtn->setIconSize(QSize(26,26));
        ui->delegateBtn->setStyleSheet("background:transparent;color:white");
        ui->delegateBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->contactBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/contactBtn_unselected.png"));
        ui->contactBtn->setIconSize(QSize(23,22));
        ui->contactBtn->setStyleSheet("background:transparent;color:grey");
        ui->contactBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        break;

    case 5:
        ui->surveyBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/surveyBtn_unselected.png"));
        ui->surveyBtn->setIconSize(QSize(29,28));
        ui->surveyBtn->setStyleSheet("background:transparent;color:grey");
        ui->surveyBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->accountBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/accountBtn_unselected.png"));
        ui->accountBtn->setIconSize(QSize(24,17));
        ui->accountBtn->setStyleSheet("background:transparent;color:grey");
        ui->accountBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->delegateBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/delegateBtn_unselected.png"));
        ui->delegateBtn->setIconSize(QSize(26,26));
        ui->delegateBtn->setStyleSheet("background:transparent;color:grey");
        ui->delegateBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->transferBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/transferBtn_unselected.png"));
        ui->transferBtn->setIconSize(QSize(23,18));
        ui->transferBtn->setStyleSheet("background:transparent;color:grey");
        ui->transferBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        ui->contactBtn->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/contactBtn.png"));
        ui->contactBtn->setIconSize(QSize(23,22));
        ui->contactBtn->setStyleSheet("background:transparent;color:white");
        ui->contactBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        break;

    default:
        break;
    }
}

void FunctionBar::retranslator()
{
    ui->retranslateUi(this);
	ui->versionLabel_2->setText("Achain Wallet\r\n" ACHAIN_WALLET_VERSION_STR);
}

void FunctionBar::getInfo(QString result)
{
	if( result.isEmpty() )  return;

    int pos = result.indexOf( "\"blockchain_head_block_age\":") + 28;
    QString seconds = result.mid( pos, result.indexOf("\"blockchain_head_block_timestamp\":") - pos - 1);

    int pos2 = result.indexOf( "\"blockchain_head_block_num\":") + 28;
    QString num = result.mid( pos2, result.indexOf("\"blockchain_head_block_age\":") - pos2 - 1);

    if( seconds.toInt() < 0)  seconds = "0";
    int numToSync = seconds.toInt()/ 10;

	if (numToSync < 20 && num.toInt() > 0) {
		updated = true;
		if (!scan) {
			scan = true;
			DataMgr::getInstance()->walletRescanBlockchain();
		}
		DataMgr::getInstance()->setUpdated();
	} else {
		updated = false;
		wait_for_update = numToSync;
	}

}

void FunctionBar::refreshResetCombobox()
{
	const QVector<CurrencyInfo>& currencyList = DataMgr::getInstance()->getCurrencyList();

	for (int i = 0; i < currencyList.size(); i++)
	{
		if (currencyList[i].name != ui->assertComboBox->itemText(i))
			ui->assertComboBox->insertItem(i, currencyList[i].name, QVariant(currencyList[i].id));
	}
}
