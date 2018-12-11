#include "accountdetailwidget.h"
#include "ui_accountdetailwidget.h"
#include "../exportdialog.h"
#include "../commondialog.h"
#include "../chooseupgradedialog.h"
#include "../extra/dynamicmove.h"
#include "datamgr.h"
#include "rpcmgr.h"

#include <QDateTime>
#include <QDebug>
#include <QClipboard>
#include <QTimeZone>

#define ACT_FEE (0.01)

AccountDetailWidget::AccountDetailWidget( QWidget *parent) :
    QWidget(parent),
    accountName(""),
    salary(0),
    produceOrNot(false),
    ui(new Ui::AccountDetailWidget) {
    ui->setupUi(this);
    setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(246, 247, 249));
    setPalette(palette);
    QString language = DataMgr::getInstance()->getSettings()->value("/settings/language").toString();
    
    if( language.isEmpty() || language == "English") {
        delegateLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/delegateLabel_En.png";
        registeredLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/registeredLabel_En.png";
        
    } else {
        delegateLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/delegateLabel.png";
        registeredLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/registeredLabel.png";
    }
    
    ui->recentTransactionTableWidget->installEventFilter(this);
    ui->recentTransactionTableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->recentTransactionTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->recentTransactionTableWidget->setFocusPolicy(Qt::NoFocus);
    ui->recentTransactionTableWidget->setShowGrid(false);
    ui->recentTransactionTableWidget->setFrameShape(QFrame::NoFrame);
    ui->recentTransactionTableWidget->setMouseTracking(true);
    ui->recentTransactionTableWidget->horizontalHeader()->setSectionsClickable(false);
    ui->recentTransactionTableWidget->horizontalHeader()->setStyleSheet("QHeaderView{background:transparent;}"
            "QHeaderView::section{background-color:rgb(246,247,249);border:0px solid #ffffff;}");
    ui->recentTransactionTableWidget->horizontalHeader()->setFixedHeight(25);
    ui->recentTransactionTableWidget->horizontalHeader()->setVisible(true);
    ui->recentTransactionTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    
    for( int i = 0; i < 3; i++) {
        QTableWidgetItem* columnHeaderItem = ui->recentTransactionTableWidget->horizontalHeaderItem(i);// 获得水平方向表头的Item对象
        columnHeaderItem->setTextColor(QColor(134, 134, 134)); // 设置文字颜色
        QFont font = columnHeaderItem->font();
        font.setPixelSize(11);
        columnHeaderItem->setFont(font);
    }
    
    ui->recentTransactionTableWidget->setColumnWidth(0, 162);
    ui->recentTransactionTableWidget->setColumnWidth(1, 173);
    ui->recentTransactionTableWidget->setColumnWidth(2, 140);
    ui->closeBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/close4.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()));
    QLabel* exportBtnLabel = new QLabel(ui->exportBtn);
    exportBtnLabel->setGeometry(55, 5, 13, 13);
    exportBtnLabel->setPixmap(QPixmap(DataMgr::getDataMgr()->getWorkPath() + "pic2/export_icon.png"));
    exportBtnLabel->show();
    ui->exportBtn->setStyleSheet("QToolButton{background-color:#ffffff;color:rgb(123,123,123);border:1px solid rgb(221,221,221);border-radius:3px;}"
                                 "QToolButton:hover{color:rgb(150,150,150);}");
    ui->copyBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/copyBtn.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}"
                                       "QToolButton:hover{background-image:url(%2pic2/copyBtn_hover.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()).arg(DataMgr::getDataMgr()->getWorkPath()));
    ui->copyBtn->setToolTip(tr("copy to clipboard"));
    ui->upgradeBtn->setStyleSheet("QToolButton{background-color:#469cfc;color:#ffffff;border:0px solid rgb(64,153,255);border-radius:3px;}"
                                  "QToolButton:hover{background-color:#62a9f8;}"
                                  "QToolButton:disabled{background-color:#cecece;}");
    ui->withdrawBtn->setStyleSheet("QToolButton{background-color:#ffffff;color:rgb(123,123,123);border:1px solid rgb(221,221,221);border-radius:3px;}"
                                   "QToolButton:hover{color:rgb(150,150,150);}");
    ui->switchBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/switch_off.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()));
    ui->switchBtn->setToolTip(tr("Click to produce/not produce blocks."));
    ui->produceWidget->setObjectName("produceWidget");
    ui->produceWidget->setStyleSheet("#produceWidget{background-color:#ffffff;color:rgb(123,123,123);border:1px solid rgb(221,221,221);border-radius:3px;}");
    ui->produceLabel->setStyleSheet("background:transparent;color:rgb(130,130,130);");
    QLabel* transferBtnLabel = new QLabel(ui->transferBtn);
    transferBtnLabel->setGeometry(18, 6, 13, 13);
    //transferBtnLabel->setPixmap(QPixmap(DataMgr::getDataMgr()->getWorkPath() + "pic2/transfer3_icon.png"));
    transferBtnLabel->show();
    ui->transferBtn->setStyleSheet("QToolButton{background-color:#469cfc;color:#ffffff;border:0px solid rgb(64,153,255);border-radius:3px;}"
                                   "QToolButton:hover{background-color:#62a9f8;}"
                                   "QToolButton:disabled{background-color:#cecece;}");
    qrCodeWidget = new QRCodeWidget(this);
    qrCodeWidget->setGeometry(223, 53, 87, 87);
    ui->noTransactionLabel->hide();
    ui->upgradeBtn->hide();
}

AccountDetailWidget::~AccountDetailWidget() {
    delete ui;
}

void AccountDetailWidget::walletTransactionHistorySpliteWithId(QString id, QString result) {
    QString commond = "id_wallet_transaction_history_splite_";
    QString account = id.mid(commond.size());
    DataMgr::getInstance()->getCurrentTrx()->insert(account, result);
    showRecentTransactions(DataMgr::getInstance()->getCurrentTrx()->value(accountName));
}

void AccountDetailWidget::setAccount(QString name) {
    if (accountName != name) { // 如果accountname 没改 produceornot保留原值
        produceOrNot = false;
    }
    
    if (name.isEmpty()) {
        return;
    }
    
    accountName = name;
    QString showName;
    
    if (accountName.size() > 13) {
        showName = accountName.left(11) + "...";
        
    } else {
        showName = accountName;
    }
    
    ui->nameLabel->setText(showName);
    ui->nameLabel->adjustSize();
    ui->nameLabel->move((this->width() - ui->nameLabel->width()) / 2, 13);
    ui->identityLabel->move(ui->nameLabel->x() - 25, ui->nameLabel->y());
    ui->delegateRankLabel->move(ui->nameLabel->x() + ui->nameLabel->width() + 5, ui->nameLabel->y() - 5);
    QString address = DataMgr::getInstance()->getAccountInfo()->value(accountName).address;
    ui->addressLabel->setText(address);
    ui->addressLabel->adjustSize();
    ui->addressLabel2->adjustSize();
    ui->addressLabel->move((this->width() - ui->addressLabel->width()) / 2, 154);
#ifdef WIN32
    ui->addressLabel2->move(ui->addressLabel->x() - 50, ui->addressLabel->y());
#else
    ui->addressLabel2->move(ui->addressLabel->x() - 55, ui->addressLabel->y());
#endif //WIN32 //zxlrun
    ui->copyBtn->move(ui->addressLabel->x() + ui->addressLabel->width() + 9, ui->addressLabel->y() - 1);
    qrCodeWidget->setString(address);

    QString balance;
	auto currencyInfo = DataMgr::getInstance()->getCurrentCurrency();

	if (currencyInfo.isAsset()) {
		balance = DataMgr::getInstance()->getAccountInfo()->value(accountName).getBalance(currencyInfo.name);
    } else {
		balance = DataMgr::getInstance()->getAccountTokenBalance(accountName, currencyInfo.name);
    }

    qDebug() << "detail" << balance;
    ui->balanceLabel->setText(QString("<font style=\"font-size:12px\" color=#000000>" + checkZero(balance.toDouble()) +
		+ " " + DataMgr::getCurrCurrencyName() + "</font>"));
    ui->balanceLabel->adjustSize();

	if (DataMgr::getCurrCurrencyName() == "ACT") {
        if (balance.toDouble() < 0.010009 - 0.000001) {
            ui->transferBtn->setEnabled(false);
            ui->upgradeBtn->setEnabled(false);

        } else if (balance.toDouble() < 0.010009 + 0.000001) {
            ui->transferBtn->setEnabled(false);
            ui->upgradeBtn->setEnabled(true);

        } else {
            ui->transferBtn->setEnabled(true);
            ui->upgradeBtn->setEnabled(true);
        }
    }
    else {
        //QString balanceACT = DataMgr::getInstance()->getAccountInfo()->value(accountName).balance;
		QString balanceACT = DataMgr::getInstance()->getAccountInfo()->value(accountName).getBalance(COMMONASSET);
        balanceACT.remove(" ACT");
        if (balanceACT.toDouble() < 0.019 - 0.000001) {
            ui->transferBtn->setEnabled(false);
            ui->upgradeBtn->setEnabled(false);

        } else if (balanceACT.toDouble() < 0.019 + 0.000001) {
            ui->transferBtn->setEnabled(false);
            ui->upgradeBtn->setEnabled(true);

        } else if (balance.toDouble() < 0.000009 - 0.000001) {
            ui->transferBtn->setEnabled(false);
            ui->upgradeBtn->setEnabled(false);

        } else if (balance.toDouble() < 0.000009 + 0.000001) {
            ui->transferBtn->setEnabled(false);
            ui->upgradeBtn->setEnabled(true);

        } else {
            ui->transferBtn->setEnabled(true);
            ui->upgradeBtn->setEnabled(true);
        }
    }
    setAssetType();

	if (currencyInfo.isAsset()) {
        showRecentTransactions(DataMgr::getInstance()->getCurrentTrx()->value(accountName));
		DataMgr::getDataMgr()->walletTransactionHistorySpliteWithId(accountName, currencyInfo.name, 0, 2);
        connect(DataMgr::getInstance(), &DataMgr::onWalletTransactionHistorySpliteWithId, this, &AccountDetailWidget::walletTransactionHistorySpliteWithId);
        
    } else {
		DataMgr::getInstance()->getTokenTrxConnect()->connectToBlockBrower(DataMgr::getInstance()->getAccountInfo()->value(accountName).address, currencyInfo.contractId, 1);
        connect(DataMgr::getInstance()->getTokenTrxConnect(), &TokenTransaction::tokenTrxRequestEnd, this, &AccountDetailWidget::showRecentTokenTransactions);
    }
    
    //ui->upgradeBtn->show();
    //ui->identityLabel->setPixmap(QPixmap(registeredLabelString));
    ui->produceWidget->hide();
    ui->delegateRankLabel->hide();
    ui->salaryLabel->hide();
    ui->salaryLabel2->hide();
    ui->withdrawBtn->hide();
    ui->balanceLabel2->move((this->width() - ui->balanceLabel->width() - 30) / 2, 195);
#ifdef WIN32
    ui->balanceLabel->move(ui->balanceLabel2->x() + 50, 195);
#else
    ui->balanceLabel->move(ui->balanceLabel2->x() + 52, 198);
#endif // WIN32 //zxlrun
    ui->transferBtn->move(225, 230);
}

void AccountDetailWidget::showRecentTokenTransactions() {
    int i = 0;
    int size =  DataMgr::getInstance()->getTokenTrxConnect()->trxvector.trx.size();
    int rowCount = size;
    
    if (rowCount > 3) {
        rowCount = 3;  // 最多显示3条
        ui->moreTransactionBtn->show();
        
    } else {
        ui->moreTransactionBtn->hide();
    }
    
    ui->recentTransactionTableWidget->setRowCount(rowCount);
    
    foreach(Transaction t, DataMgr::getInstance()->getTokenTrxConnect()->trxvector.trx) {
        ui->recentTransactionTableWidget->setRowHeight(i, 25);
        QString fromAddr = t.from_addr;
        QString toAddr = t.to_addr;
        QString fromAccount = DataMgr::getInstance()->getAddrAccont(fromAddr);
        QString toAccount = DataMgr::getInstance()->getAddrAccont(toAddr);
        QString current_addr = DataMgr::getInstance()->getAccountAddr(accountName);
        QString amount = t.amount;
        QDateTime time = QDateTime::fromString(t.trx_time, "yyyy-MM-dd hh:mm:ss");
        time.setTimeZone(QTimeZone::utc());
        time = time.toTimeZone(QTimeZone::systemTimeZone());
        QString currentDateTime = time.toString("yyyy-MM-dd hh:mm:ss");
        ui->recentTransactionTableWidget->setItem(i, 0, new QTableWidgetItem(currentDateTime));
        QTableWidgetItem* item;
        bool isOut = false;
        bool isSelf = false;

        if ((fromAddr == toAddr) && (fromAddr == current_addr)) {
            isSelf = true;
        }
        
        if( fromAddr != current_addr) {
            // 如果 fromaccount 不为本账户，则 为对方账户
            if (fromAccount.isEmpty()) {
                item = new QTableWidgetItem(fromAddr);
                
            } else {
                item = new QTableWidgetItem(fromAccount);
                item->setTextColor(QColor(64, 154, 255));
            }
            
        } else {
            // 如果 fromaccount 为本账户， 则toaccount  为对方账户
            if (toAccount.isEmpty()) {
                item = new QTableWidgetItem(toAddr);
                
            } else {
                item = new QTableWidgetItem(toAccount);
                item->setTextColor(QColor(64, 154, 255));
            }
            
            isOut = true;
        }
        
        ui->recentTransactionTableWidget->setItem(i, 1, item);
        
        if (isSelf) {
            QTableWidgetItem* item = new QTableWidgetItem("-/+" + checkZero(amount.toDouble()));
            item->setTextColor(QColor(255, 80, 63));
            ui->recentTransactionTableWidget->setItem(i, 2, item);
        } else {
            // amount
            if(isOut) {
                QTableWidgetItem* item = new QTableWidgetItem("-" + checkZero(amount.toDouble()));
                item->setTextColor(QColor(255, 80, 63));
                ui->recentTransactionTableWidget->setItem(i, 2, item);

            } else {
                QTableWidgetItem* item = new QTableWidgetItem("+" + checkZero(amount.toDouble()));
                item->setTextColor(QColor(71, 178, 156));
                ui->recentTransactionTableWidget->setItem(i, 2, item);
            }
        }

        i++;
    }
}

struct ShowString {
	QString time_stamp;
	QString account;
	QString amount;
	QString fee;
	QString balance;
	QString memo;
	int trx_type;
	bool isOut;
	bool self;
	bool con;
};

void AccountDetailWidget::showRecentTransactions(QString result) {

    if (result == "\"result\":[]" || result.isEmpty()) {
        ui->recentTransactionTableWidget->hide();
        ui->noTransactionLabel->show();
        ui->moreTransactionBtn->hide();
        return;
        
    } else {
        ui->recentTransactionTableWidget->show();
        ui->noTransactionLabel->hide();
    }
    
    QVector <TrxResult> trxResult;
    DataMgr::getInstance()->parseTrxResultJson( QString("{")+ result + QString("}"), trxResult);

    QVector <ShowString> showAccountList;
	QString account = accountName;
	QString asset_type = DataMgr::getCurrCurrencyName();
	QString _from_account("");
	QString _to_account("");
    
	foreach(TrxResult trx, trxResult) {
		ShowString showString = { "--", "--", "--", "--", "--", "--", 0, false, false, false };
		showString.time_stamp = trx.time_stamp;
		showString.trx_type = trx.trx_type;

        if (trx.entries.at(0).memo == "withdraw exec cost") {
            if (trx.entries.at(0).to_account.isEmpty()) {
				showString.con = true;
				showString.isOut = true;
            }
        }

		const LedgerEntries& ledger = trx.entries.at(trx.entries.size() - 1);

		_from_account = trx.entries.at(0).from_account;
		_to_account = ledger.to_account;
		if (trx.entries.size() == 2 && !trx.entries.at(1).to_account.isEmpty())
		{
			_to_account = trx.entries.at(1).to_account;
		}

		if (!_to_account.isEmpty() && _to_account != account)
			showString.isOut = true;

		if (_from_account == account) {
			showString.account = _to_account;
			const QString& _account_addr = DataMgr::getInstance()->getAddrAccont(_to_account);
			if (!_account_addr.isEmpty())
				showString.account = _account_addr;
		}
		else {
			showString.account = _from_account;
			const QString& _account_addr = DataMgr::getInstance()->getAddrAccont(ledger.from_account);
			if (!_account_addr.isEmpty())
				showString.account = _account_addr;
		}

		if (showString.account == account) {
			showString.self = true;
		}

		showString.amount = doubleToStr(ledger.amount.amount);
		showString.balance = ledger.getBalance(account, asset_type);

		showString.memo = trx.entries.at(0).memo;
		if (showString.memo.isEmpty() && trx.entries.size() > 1)
			showString.memo = trx.entries.at(1).memo;

        showString.fee = QString::number(trx.fee.amount);
        
        if (showString.con) {
            showString.memo = tr("withdraw exec cost");
            showString.fee = "--";
            showString.account = "CON";
        }
        
        showAccountList.append(showString);
    }
    
	int listSize = showAccountList.size();
	int rowCount = listSize;

    if (rowCount > 3) {
        rowCount = 3;  // 最多显示3条
        ui->moreTransactionBtn->show();
    } else {
        ui->moreTransactionBtn->hide();
    }
    
    ui->recentTransactionTableWidget->setRowCount(rowCount);
    
    for (int i = rowCount - 1; i > -1; i--) {
        ui->recentTransactionTableWidget->setRowHeight(i, 25);
		ShowString str = showAccountList.at(listSize - (i + 1));

        QTableWidgetItem* item = new QTableWidgetItem(str.account);
        if( str.account.mid(0, 3) != "ACT") {
            item->setTextColor(QColor(64, 154, 255));
        }
        ui->recentTransactionTableWidget->setItem(i, 1, item);

        // 时间
        QDateTime time = QDateTime::fromString(str.time_stamp, "yyyy-MM-ddThh:mm:ss");
        time.setTimeZone(QTimeZone::utc());
        time = time.toTimeZone(QTimeZone::systemTimeZone());
        QString currentDateTime = time.toString("yyyy-MM-dd hh:mm:ss");
        ui->recentTransactionTableWidget->setItem(i, 0, new QTableWidgetItem(currentDateTime));

        // 金额
		double amount = str.amount.toDouble() / DataMgr::assetPrecision();
		QString showAmount;
		QTableWidgetItem* amount_item = new QTableWidgetItem();

		bool is_multi_asset_trx = (str.trx_type == TransactionType::transfer_multi_asset
			&& DataMgr::getCurrCurrencyName() == COMMONASSET);
		bool isReg = str.trx_type == TransactionType::register_account_transaction;

		if (is_multi_asset_trx)
		{
			double fee = str.fee.toDouble() / 100000;
			showAmount = QString("-" + checkZero(fee));
			if (showAmount == "-0") {
				showAmount = QString("0");
			}
			amount_item->setTextColor(QColor(255, 80, 63));
		}
		else if (isReg || (amount > -0.000001 && amount < 0.000001))
		{
			showAmount = QString("0");
			amount_item->setTextColor(QColor(255, 80, 63));
        }
        else if (str.isOut && !str.self) {
            showAmount = QString("-" + checkZero(amount));
            if (showAmount == "-0") {
                showAmount = QString("0");
            }
			amount_item->setTextColor(QColor(255, 80, 63));
        }
		else if(!str.self && !str.isOut) {
			showAmount = QString("+" + checkZero(amount));
			amount_item->setTextColor(QColor(71, 178, 156));
		}
		else {
			showAmount = QString("-/+" + checkZero(amount));
			amount_item->setTextColor(QColor(255, 80, 63));
		}

		amount_item->setText(showAmount);
		ui->recentTransactionTableWidget->setItem(i, 2, amount_item);
    }
}

void AccountDetailWidget::refresh() {
}

void AccountDetailWidget::dynamicShow() {
    show();
    DynamicMove* dynamicMove = new DynamicMove( this, QPoint(297, 93), 20, 10, this);
    dynamicMove->start();
    setAssetType();
}
void AccountDetailWidget::dynamicHide() {
    DynamicMove* dynamicMove = new DynamicMove( this, QPoint(826, 93), 20, 10, this);
    connect( dynamicMove, SIGNAL(moveEnd()), this, SLOT(moveEnd()));
    dynamicMove->start();
}
void AccountDetailWidget::on_closeBtn_clicked() {
    emit back();
}
void AccountDetailWidget::on_moreTransactionBtn_clicked() {
    emit showAccountPage(accountName);
}
void AccountDetailWidget::on_exportBtn_clicked() {
    ExportDialog exportDialog(accountName);
    exportDialog.pop();
}
void AccountDetailWidget::on_transferBtn_clicked() {
    bool updated = DataMgr::getInstance()->isUpdate();
    
    if (!updated) {
        CommonDialog commonDialog(CommonDialog::OkOnly);
        commonDialog.setText(tr("Please wait for wallet to sync"));
        commonDialog.pop();
        
    } else {
        emit showTransferPage(accountName);
    }
}
void AccountDetailWidget::on_copyBtn_clicked() {
    QClipboard* clipBoard = QApplication::clipboard();
    clipBoard->setText(ui->addressLabel->text());
    CommonDialog commonDialog(CommonDialog::OkOnly);
    commonDialog.setText(tr("Copy to clipboard"));
    commonDialog.pop();
}
void AccountDetailWidget::on_upgradeBtn_clicked() {
    // 如果是已注册账户
    {
        ChooseUpgradeDialog* chooseUpgradeDialog = new ChooseUpgradeDialog(accountName, this);
        chooseUpgradeDialog->move( ui->upgradeBtn->mapToGlobal( QPoint(-40, 30) ) );
        connect( chooseUpgradeDialog, SIGNAL(upgrade(QString)), this, SIGNAL(upgrade(QString)));
        connect( chooseUpgradeDialog, SIGNAL(applyDelegate(QString)), this, SIGNAL(applyDelegate(QString)));
        chooseUpgradeDialog->exec();
    }
}
void AccountDetailWidget::on_withdrawBtn_clicked() {
    double amount = salary - 0.01;
    
    if( amount > 0.009999) {
        CommonDialog commonDialog(CommonDialog::OkAndCancel);
        commonDialog.setText( tr("Sure to withdraw your salary?"));
    }
}
void AccountDetailWidget::on_switchBtn_clicked() {
    CommonDialog commmonDialog(CommonDialog::OkAndCancel);
}
void AccountDetailWidget::moveEnd() {
    // 移出mainpage显示范围的时候 最后一帧会导致mainpage挡住bottombar
    // 先隐藏 dynamicshow时再显示
    hide();
}
void AccountDetailWidget::setAssetType() {
    QTableWidgetItem *thirdHeadItem = ui->recentTransactionTableWidget->horizontalHeaderItem(2);
    if (NULL != thirdHeadItem) {
        QString curText = thirdHeadItem->text();
        int sep = curText.indexOf('(');
        if (sep > 0) {
            thirdHeadItem->setText(curText.left(sep + 1) + DataMgr::getCurrCurrencyName() + ")");
        }
    }
}
