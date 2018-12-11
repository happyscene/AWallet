#include <QDebug>
#include <QPainter>
#include <QHelpEvent>
#include <QDateTime>
#include <QTextCodec>
#include <QScrollBar>
#include <QFont>
#include <QTimeZone>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

#include "accountpage.h"
#include "ui_accountpage.h"
#include "goopal.h"
#include "debug_log.h"
#include <QClipboard>
#include "commondialog.h"
#include "showcontentdialog.h"
#include "chooseupgradedialog.h"
#include "control/remarkcellwidget.h"
#include "datamgr.h"
#include "frame.h"

AccountPage::AccountPage(QString name, QWidget *parent) :
    QWidget(parent),
    accountName(name),
    transactionType(0),
    address(""),
    inited(false),
    ui(new Ui::AccountPage)
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    ui->setupUi(this);
    setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(255, 255, 255));
    setPalette(palette);
    mutexForAddressMap.lock();
    
    if( accountName.isEmpty()) { // 如果是点击账单跳转
        {
            mutexForAddressMap.unlock();
            emit back();
            return;
        }
    }
    
    // 账户下拉框按字母顺序排序
    QStringList keys = DataMgr::getInstance()->getAccountInfo()->keys();
    ui->accountComboBox->addItems( keys);
    
    if( accountName.isEmpty() ) {
        ui->accountComboBox->setCurrentIndex(0);
        
    } else {
        ui->accountComboBox->setCurrentText( accountName);
    }
    
    mutexForAddressMap.unlock();
    ui->pageLineEdit->setStyleSheet("QLineEdit{border-width:0;border-style:outset}");
    ui->pageLineEdit->setText("1");
    QIntValidator *validator = new QIntValidator(1, 9999, this);
    ui->pageLineEdit->setValidator(validator);
    ui->pageLineEdit->setAttribute(Qt::WA_InputMethodEnabled, false);
    ui->pageLineEdit->setDisabled(true);
    currentPageIndex = 1;
    ui->pageLineEdit->setText(QString::number(currentPageIndex));
    ui->prePageBtn->setStyleSheet("QToolButton:!hover{border:0px;color:#999999;} QToolButton:hover{border:0px;color:#469cfc;}");
    ui->nextPageBtn->setStyleSheet("QToolButton:!hover{border:0px;color:#999999;} QToolButton:hover{border:0px;color:#469cfc;}");
#ifdef WIN32
    ui->accountComboBox->setStyleSheet(QString("QComboBox{border: 1px solid gray;border-radius: 3px;padding: 1px 2px 1px 8px;min-width: 9em;}"
                                       "QComboBox::drop-down {subcontrol-origin: padding;subcontrol-position: top right;width: 20px;"
                                       "border-left-width: 1px;border-left-color: darkgray;border-left-style: solid;"
                                       "border-top-right-radius: 3px;border-bottom-right-radius: 3px;}"
                                       "QComboBox::down-arrow {image: url(%1pic2/comboBoxArrow.png);}").arg(DataMgr::getDataMgr()->getWorkPath())
                                      );
#else
    ui->accountComboBox->setStyleSheet(QString(
                                           "QComboBox {"
                                           "background-color: rgba(47, 47, 53, 10);"
                                           "background-repeat: no-repeat; "
                                           "selection-background-color: rgb(85, 85, 255); "
                                           "selection-color: rgb(255, 255, 255); "
                                           "}"
                                           "QComboBox::drop-down {background-color: rgba(47, 47, 53, 10);}"
                                           "QComboBox::down-arrow {image: url(%1pic2/comboBoxArrow.png);}"
                                       ).arg(DataMgr::getDataMgr()->getWorkPath())
                                      );
#endif // WIN32//zxlrun
    ui->transactionTypeComboBox->setStyleSheet(QString("QComboBox{border: 1px solid gray;border-radius: 3px;padding: 1px 2px 1px 8px;min-width: 5em;}"
            "QComboBox::drop-down {subcontrol-origin: padding;subcontrol-position: top right;width: 20px;"
            "border-left-width: 1px;border-left-color: darkgray;border-left-style: solid;"
            "border-top-right-radius: 3px;border-bottom-right-radius: 3px;}"
            "QComboBox::down-arrow {image: url(%1pic2/comboBoxArrow.png);}").arg(DataMgr::getDataMgr()->getWorkPath())
                                              );
    ui->copyBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/copyBtn.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}"
                                       "QToolButton:hover{background-image:url(%2pic2/copyBtn_hover.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()).arg(DataMgr::getDataMgr()->getWorkPath()));
    ui->copyBtn->setToolTip(tr("copy to clipboard"));
    ui->accountTransactionsTableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    ui->accountTransactionsTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->accountTransactionsTableWidget->setFocusPolicy(Qt::NoFocus);
    ui->accountTransactionsTableWidget->setColumnWidth(0, 120);
    ui->accountTransactionsTableWidget->setColumnWidth(1, 130);
    ui->accountTransactionsTableWidget->setColumnWidth(2, 130);
    ui->accountTransactionsTableWidget->setColumnWidth(3, 130);
    ui->accountTransactionsTableWidget->setColumnWidth(4, 130);
    ui->accountTransactionsTableWidget->setColumnWidth(5, 120);
    ui->accountTransactionsTableWidget->setShowGrid(false);
    ui->accountTransactionsTableWidget->setStyleSheet("QTableView::item { border-bottom: 1px dashed rgb(180,180,180); }");
    ui->accountTransactionsTableWidget->setFrameShape(QFrame::NoFrame);
    ui->accountTransactionsTableWidget->setMouseTracking(true);
    ui->accountTransactionsTableWidget->setGeometry(39, 190, 770, 5 * 57);
    ui->initLabel->hide();
    ui->label_2->hide();
    ui->transactionTypeComboBox->hide();
    retranslator();
    init();
    updateTransactionsList();
    showTransactions(DataMgr::getInstance()->getCurrentTrx()->value(accountName));
    balance();
    inited = true;
    connect(DataMgr::getInstance(), &DataMgr::onWalletTransactionHistorySpliteWithId, this, &AccountPage::walletTransactionHistorySpliteWithId);
//  connect(DataMgr::getInstance(), &DataMgr::onWalletTransactionHistorySplite, this, &AccountPage::walletTransactionHistorySplite);

	Frame* frame = dynamic_cast<Frame*>(Goopal::getInstance()->mainFrame);
	connect(frame, &Frame::updateTotalBalance, this, &AccountPage::balance);

    DLOG_QT_WALLET_FUNCTION_END;
}

QString discard(const QString &str) {
    int dotPos = str.indexOf(".");
    
    if( dotPos != -1) {
        return str.left( dotPos + 3);
        
    } else {
        DLOG_QT_WALLET(" no dot!");
        return NULL;
    }
}

AccountPage::~AccountPage() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    delete ui;
    DLOG_QT_WALLET_FUNCTION_END;
}

void AccountPage::init() {
    if (accountName == "") {
        return;
    }
    
    ui->label_2->hide();
    ui->transactionTypeComboBox->hide();
    QString showName = DataMgr::getInstance()->getAccountInfo()->value(accountName).name;
    showName = showName.left(11) + "...";
    address =  DataMgr::getInstance()->getAccountInfo()->value(accountName).address;
	ui->addressLabel->setText(address.left(11) + "...");

	int height = ui->addressLabel->geometry().height();
	ui->addressLabel->adjustSize();
	QRect labelGeometry = ui->addressLabel->geometry();
	labelGeometry.setHeight(height);
	ui->addressLabel->setGeometry(labelGeometry);

    ui->copyBtn->move(ui->addressLabel->x() + ui->addressLabel->width() + 9, 53);
    ui->accountComboBox->setCurrentText(accountName);
    /*
    // 如果是已注册账户
    if( Goopal::getInstance()->registerMapValue(accountName) != "NO"
            && !Goopal::getInstance()->registerMapValue(accountName).isEmpty())
    {
        // 如果是代理
        if( Goopal::getInstance()->delegateAccountList.indexOf(accountName) != -1)
        {
            ui->identityLabel->setPixmap(QPixmap(delegateLabelString));
    
            //Goopal::getInstance()->postRPC( toJsonFormat( "id_wallet_get_delegate_statue_" + accountName, "wallet_get_delegate_statue", QStringList() << accountName ));
        }
        else // 如果不是代理
        {
            ui->identityLabel->setPixmap(QPixmap(registeredLabelString));
        }
    }
    else  // 如果账户未升级
    */
    {
        ui->identityLabel->setPixmap(QPixmap(""));
    }
}

void AccountPage::updateTransactionsList() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    
    if (accountName.size() > 0) {
		auto currencyInfo = DataMgr::getCurrentCurrency();
		if (currencyInfo.isAsset()) {
			DataMgr::getDataMgr()->walletTransactionHistorySpliteWithId(accountName, currencyInfo.name, 0, 2);
        } else {
			showTokenTransactions(accountName, currencyInfo.contractId);
        }
    }
    
    DLOG_QT_WALLET_FUNCTION_END;
}

void AccountPage::refresh() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    init();
    updateTransactionsList();
    balance();
    setAssetType();
    DLOG_QT_WALLET_FUNCTION_END;
}

bool isExistInWallet(QString);

void AccountPage::on_copyBtn_clicked() {
    QClipboard* clipBoard = QApplication::clipboard();
    clipBoard->setText(address);
    CommonDialog commonDialog(CommonDialog::OkOnly);
    commonDialog.setText(tr("Copy to clipboard"));
    commonDialog.pop();
}

void AccountPage::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setPen(QPen(QColor(177, 183, 188), Qt::SolidLine));
    painter.setBrush(QBrush(QColor(229, 229, 229), Qt::SolidPattern));
    painter.drawRect(-1, -1, 828, 92);
    painter.drawLine(QPoint(30, 150), QPoint(796, 150));
}

void AccountPage::on_accountComboBox_currentIndexChanged(const QString &arg1) {
    if( inited) { // 防止accountComboBox当前值改变的时候触发
        accountName = arg1;
        currentPageIndex = 1;
        ui->pageLineEdit->setText(QString::number(currentPageIndex));
        init();
        balance();
        DataMgr::getInstance()->setCurrentAccount(accountName);
        showTransactions(DataMgr::getInstance()->getCurrentTrx()->value(accountName));
    }
}

void AccountPage::retranslator() {
    ui->retranslateUi(this);
    QString language = DataMgr::getInstance()->getSettings()->value("/settings/language").toString();
    
    if( language.isEmpty() || language == "English") {
        ui->label_3->move(601, 122);
        ui->label_6->move(614, 119);
        ui->label_6->adjustSize();
        ui->label_4->move(695, 122);
        ui->label_7->move(708, 119);
        ui->label_7->adjustSize();
        delegateLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/delegateLabel_En.png";
        registeredLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/registeredLabel_En.png";
        
    } else {
        ui->label_3->move(651, 122);
        ui->label_6->move(664, 119);
        ui->label_6->adjustSize();
        ui->label_4->move(711, 122);
        ui->label_7->move(724, 119);
        ui->label_7->adjustSize();
        delegateLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/delegateLabel.png";
        registeredLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/registeredLabel.png";
    }
}

void AccountPage::balance()
{
    QString amount;
	auto currencyInfo = DataMgr::getCurrentCurrency();
	if (currencyInfo.isAsset()) {
		amount = DataMgr::getInstance()->getAccountInfo()->value(accountName).getBalance(currencyInfo.name);
    } else {
		amount = DataMgr::getInstance()->getAccountTokenBalance(accountName, currencyInfo.name);
    };

    ui->balanceLabel->setText("<body><font style=\"font-size:20px\" color=#000000>" + checkZero(amount.toDouble()) + "</font><font style=\"font-size:12px\" color=#000000> "
		+ DataMgr::getCurrCurrencyName() + "</font></body>");

	int height = ui->balanceLabel->geometry().height();
	ui->balanceLabel->adjustSize();
	QRect labelGeometry = ui->balanceLabel->geometry();
	labelGeometry.setHeight(height);
	ui->balanceLabel->setGeometry(labelGeometry);

    ui->balanceLabel->move(555, 44);
    ui->label_16->move(553 - ui->label_16->width(), 42);
    return;
}

void AccountPage::walletTransactionHistorySpliteWithId(QString id, QString result) {
    QString commond = "id_wallet_transaction_history_splite_";
    QString account = id.mid(commond.size());
    
    if (account.size() > 0) {
        DataMgr::getInstance()->getCurrentTrx()->insert(account, result);
		showCommonTransactions(account, result);
    }
}

void AccountPage::on_transactionTypeComboBox_currentIndexChanged(int index) {
    currentPageIndex = 1;
    ui->pageLineEdit->setText(QString::number(currentPageIndex));
    showTransactions(accountName);
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

void AccountPage::showCommonTransactions(QString account, QString result) {
    int _split_type = ui->transactionTypeComboBox->currentIndex();
    
    if (result == "\"result\":[]" || result.isEmpty()) {
        ui->accountTransactionsTableWidget->setRowCount(0);
        ui->prePageBtn->hide();
        ui->numberLabel->hide();
        ui->pageLineEdit->hide();
        ui->pageLabel->hide();
        ui->nextPageBtn->hide();
        
        if( result == "\"result\":[]") {
            ui->initLabel->show();
            
        } else {
            ui->initLabel->hide();
        }
        
        return;
    }
    
    QVector <TrxResult> trxResult;
    DataMgr::getInstance()->parseTrxResultJson( QString("{")+ result + QString("}"), trxResult);
    ui->prePageBtn->show();
    ui->numberLabel->show();
    ui->pageLineEdit->show();
    ui->pageLabel->show();
    ui->nextPageBtn->show();
    ui->initLabel->hide();

    QVector <ShowString> showAccountList;
	QString asset_type = DataMgr::getCurrCurrencyName();
	QString _from_account("");
	QString _to_account("");

    foreach (TrxResult trx, trxResult) {
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

		if (_split_type == 0)
		{
			if (!_to_account.isEmpty() && _to_account != account)
				showString.isOut = true;
		}
		if (_split_type == 1)
		{
			if (!_to_account.isEmpty() && _to_account != account)
				continue;
			showString.isOut = true;
		}
		else if (_split_type == 2)
		{
			if (!_from_account.isEmpty() && _from_account != account)
				continue;
			showString.isOut = false;
		}

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

		showString.fee = doubleToStr(trx.fee.amount);
        
        if (showString.con) {
            showString.memo = tr("withdraw exec cost");
            showString.fee = "--";
            showString.account = "CON";
        }

        showAccountList.append(showString);
    }

	int size = showAccountList.size();
	const QVector<ShowString>::const_iterator& showAccountList_iter = showAccountList.constBegin();
    
    ui->numberLabel->setText( tr("total ") + QString::number( size) + tr(" ,"));
    ui->pageLabel->setText( "/" + QString::number( (size - 1)/10 + 1 ) );
    int rowCount = size - (currentPageIndex - 1) * 10;
    
    if( rowCount > 10 )
        rowCount = 10;  // 一页最多显示10行
        
    ui->accountTransactionsTableWidget->setRowCount(rowCount);
    //  ui->accountTransactionsTableWidget->clear();
    
    for(int i = rowCount - 1; i > -1; i--) {
        ShowString str;
        int next_ledger = size - (i + 1) - (currentPageIndex - 1) * 10;
        str = *(showAccountList_iter + next_ledger);
        ui->accountTransactionsTableWidget->setRowHeight(i, 57);

        QTableWidgetItem* item = new QTableWidgetItem(str.account);
        if( str.account.mid(0, 3) != "ACT") {
            item->setTextColor(QColor(64, 154, 255));
        }
        ui->accountTransactionsTableWidget->setItem(i, 1, item);

        // 时间
        QDateTime time = QDateTime::fromString(str.time_stamp, "yyyy-MM-ddThh:mm:ss");
        time.setTimeZone(QTimeZone::utc());
        time = time.toTimeZone(QTimeZone::systemTimeZone());
        QString currentDateTime = time.toString("yyyy-MM-dd\r\nhh:mm:ss");
        ui->accountTransactionsTableWidget->setItem(i, 0, new QTableWidgetItem(currentDateTime));

        // 手续费
		bool is_multi_asset_trx = (str.trx_type == TransactionType::transfer_multi_asset
			&& DataMgr::getCurrCurrencyName() == COMMONASSET);

		if (is_multi_asset_trx)
		{
			ui->accountTransactionsTableWidget->setItem(i, 3, new QTableWidgetItem("--"));
		}
		else
		{
			double fee = str.fee.toDouble() / 100000;
			if (fee > 0.000001)
				ui->accountTransactionsTableWidget->setItem(i, 3, new QTableWidgetItem(checkZero(fee)));
			else
				ui->accountTransactionsTableWidget->setItem(i, 3, new QTableWidgetItem("--"));
		}

        // 金额
		double amount = str.amount.toDouble() / DataMgr::assetPrecision();
        QString showAmount;
		QTableWidgetItem* amount_item = new QTableWidgetItem();

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
		else if(isReg || (amount > -0.000001 && amount < 0.000001))
		{
			showAmount = QString("0");
			amount_item->setTextColor(QColor(255, 80, 63));
		}
		else if (!str.self && str.isOut)
		{
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
        ui->accountTransactionsTableWidget->setItem(i, 2, amount_item);

        // 如果是自己转自己的 支出部分 余额里面把 交易金额扣除
		QString balance = checkZero(str.balance.toDouble() / DataMgr::assetPrecision());
        QTableWidgetItem* balance_item = new QTableWidgetItem(balance);
        ui->accountTransactionsTableWidget->setItem(i, 4, balance_item);

		// 备注
		QString remark = str.memo.isEmpty() ? "--" : str.memo;
        ui->accountTransactionsTableWidget->setItem(i, 5, new QTableWidgetItem(remark));
    }
}

void AccountPage::showTokenTransactions(QString accountName, QString contractId)
{
	DataMgr::getInstance()->getTokenTrxConnect()->connectToBlockBrower(DataMgr::getInstance()->getAccountInfo()->value(accountName).address, contractId, currentPageIndex);
    connect(DataMgr::getInstance()->getTokenTrxConnect(), &TokenTransaction::tokenTrxRequestEnd, this, &AccountPage::displayTokenTrxVector);
}

void AccountPage::displayTokenTrxVector() {
    int i = 0;
    //  ui-> DataMgr::getInstance()->getTokenTrxConnect()->trxvector.totaRecords;
    int size = DataMgr::getInstance()->getTokenTrxConnect()->trxvector.totaRecords;
    int currentPage = DataMgr::getInstance()->getTokenTrxConnect()->trxvector.currentPage;
    int totalPage = DataMgr::getInstance()->getTokenTrxConnect()->trxvector.totalPage;
    ui->prePageBtn->show();
    ui->numberLabel->show();
    ui->pageLineEdit->show();
    ui->pageLabel->show();
    ui->nextPageBtn->show();
    ui->initLabel->hide();
    ui->numberLabel->setText( tr("total ") + QString::number( size) + tr(" ,"));
    ui->pageLineEdit->setText(QString::number(currentPage));
    ui->accountTransactionsTableWidget->setRowCount(DataMgr::getInstance()->getTokenTrxConnect()->trxvector.trx.size());
    ui->pageLabel->setText( "/" + QString::number(totalPage) );
    
    foreach(Transaction t, DataMgr::getInstance()->getTokenTrxConnect()->trxvector.trx) {
        ui->accountTransactionsTableWidget->setRowHeight(i, 57);
        QString fromAddr = t.from_addr;
        QString toAddr = t.to_addr;
        QString fromAccount = DataMgr::getInstance()->getAddrAccont(fromAddr);
        QString toAccount = DataMgr::getInstance()->getAddrAccont(toAddr);
        QString current_addr = DataMgr::getInstance()->getAccountAddr(accountName);
        QString amount = t.amount;
        QDateTime time = QDateTime::fromString(t.trx_time, "yyyy-MM-dd hh:mm:ss");
        time.setTimeZone(QTimeZone::utc());
        time = time.toTimeZone(QTimeZone::systemTimeZone());
        QString currentDateTime = time.toString("yyyy-MM-dd\r\nhh:mm:ss");
        ui->accountTransactionsTableWidget->setItem(i, 0, new QTableWidgetItem(currentDateTime));
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
        
        ui->accountTransactionsTableWidget->setItem(i, 1, item);
        QString showAmount;
        
        //make a function getTableWidgetItem(QStirng& showStrign, QFont& font, QColor&);
        if (isSelf) {
            showAmount = QString("-/+" + checkZero(amount.toDouble()));
            QTableWidgetItem* item = new QTableWidgetItem(showAmount);
            item->setTextColor(QColor(255, 80, 63));
            ui->accountTransactionsTableWidget->setItem(i, 2, item);
            ui->accountTransactionsTableWidget->setItem(i, 4, new QTableWidgetItem(checkZero(t.tobalance.toDouble())));
            
        } else {
            // amount
            if(isOut) {
                showAmount = QString("-" + checkZero(amount.toDouble()));
                QTableWidgetItem* item = new QTableWidgetItem(showAmount);
                item->setTextColor(QColor(255, 80, 63));
                ui->accountTransactionsTableWidget->setItem(i, 2, item);
                ui->accountTransactionsTableWidget->setItem(i, 4, new QTableWidgetItem(checkZero(t.frombalance.toDouble())));
                
            } else {
                showAmount = QString("+" + checkZero(amount.toDouble()));
                QTableWidgetItem* item = new QTableWidgetItem(showAmount);
                item->setTextColor(QColor(71, 178, 156));
                ui->accountTransactionsTableWidget->setItem(i, 2, item);
                ui->accountTransactionsTableWidget->setItem(i, 4, new QTableWidgetItem(checkZero(t.tobalance.toDouble())));
            }
        }
        
        //ui->accountTransactionsTableWidget->setItem(i,5,new QTableWidgetItem("--"));
        if (isOut) {
            ui->accountTransactionsTableWidget->setItem(i, 3, new QTableWidgetItem(checkZero(t.fee.toDouble()))); //fee
            
        } else {
            ui->accountTransactionsTableWidget->setItem(i, 3, new QTableWidgetItem("--")); //fee
        }
        
        if (t.memo.isEmpty()) {
            ui->accountTransactionsTableWidget->setItem(i, 5, new QTableWidgetItem("--"));
            
        } else {
            ui->accountTransactionsTableWidget->setItem(i, 5, new QTableWidgetItem(t.memo));
        }
        
        i++;
    }
}
void AccountPage::showTransactions(QString)
{
	auto currencyInfo = DataMgr::getCurrentCurrency();
	if (currencyInfo.isAsset()) {
        showCommonTransactions(accountName, DataMgr::getInstance()->getCurrentTrx()->value(accountName));
    } else {
		DataMgr::getInstance()->getTokenTrxConnect()->connectToBlockBrower(accountName, currencyInfo.name, currentPageIndex);
    }
}
void AccountPage::on_accountTransactionsTableWidget_cellClicked(int row, int column) {
    /*if( column == 1 ) {
        ShowContentDialog showContentDialog( ui->accountTransactionsTableWidget->item(row, column)->text(),this);
        int scrollBarValue = ui->accountTransactionsTableWidget->verticalScrollBar()->sliderPosition();
        showContentDialog.move( ui->accountTransactionsTableWidget->mapToGlobal( QPoint(120, 57 * (row - scrollBarValue) - 3)));
        showContentDialog.exec();
    
        return;
    }
    
    if( column == 5) {
    
        QString remark = ui->accountTransactionsTableWidget->item(row, column)->text();
        remark.remove(' ');
        if( remark.isEmpty() )  return;
        ShowContentDialog showContentDialog( ui->accountTransactionsTableWidget->item(row, column)->text(),this);
        int scrollBarValue = ui->accountTransactionsTableWidget->verticalScrollBar()->sliderPosition();
        int length = ui->accountTransactionsTableWidget->item(row, column)->text().length();
        if (ui->accountTransactionsTableWidget->item(row, column)->text() == "--")
        {
            showContentDialog.hide();
        }
        else if (ui->accountTransactionsTableWidget->item(row, column)->text().length() > 30)
        {
            showContentDialog.move(ui->accountTransactionsTableWidget->mapToGlobal(QPoint(595 - ui->accountTransactionsTableWidget->item(row, column)->text().length() * 5,
                57 * (row - scrollBarValue) - 3)));
        }
        else
        {
            showContentDialog.move(ui->accountTransactionsTableWidget->mapToGlobal(QPoint(630 - ui->accountTransactionsTableWidget->item(row, column)->text().length() * 5,
                57 * (row - scrollBarValue) - 3)));
        }
        showContentDialog.exec();
        return;
    }*/
}
void AccountPage::on_prePageBtn_clicked() {
    ui->accountTransactionsTableWidget->scrollToTop();
    
    if( currentPageIndex == 1)
        return;
        
    currentPageIndex--;
    showTransactions(DataMgr::getInstance()->getCurrentTrx()->value(accountName));
    ui->pageLineEdit->setText( QString::number(currentPageIndex));
    
	auto currencyInfo = DataMgr::getCurrentCurrency();
	if (currencyInfo.isAsset() == false)
	{
        ui->prePageBtn->hide();
        ui->numberLabel->hide();
        ui->pageLineEdit->hide();
        ui->pageLabel->hide();
        ui->nextPageBtn->hide();
    }
}
void AccountPage::on_nextPageBtn_clicked() {
    //    if( currentPageIndex >=  ((searchList.size() - 1)/10 + 1))  return;
    int totalPageNum = ui->pageLabel->text().remove("/").toInt();
    
    if(  currentPageIndex >= totalPageNum )  return;
    
    currentPageIndex++;
    showTransactions(DataMgr::getInstance()->getCurrentTrx()->value(accountName));
    ui->pageLineEdit->setText( QString::number(currentPageIndex));
    ui->accountTransactionsTableWidget->scrollToTop();


	auto currencyInfo = DataMgr::getCurrentCurrency();
	if (currencyInfo.isAsset() == false)
	{
        ui->prePageBtn->hide();
        ui->numberLabel->hide();
        ui->pageLineEdit->hide();
        ui->pageLabel->hide();
        ui->nextPageBtn->hide();
    }
}
void AccountPage::on_pageLineEdit_editingFinished() {
    if (ui->pageLineEdit->text().isEmpty()) {
        ui->pageLineEdit->setText(QString::number(currentPageIndex));
    }
    
    currentPageIndex = ui->pageLineEdit->text().toInt();
}
void AccountPage::on_pageLineEdit_textEdited(const QString &arg1) {
    if (arg1.isEmpty()) {
        return;
    }
    
    if( arg1.at(0) == '0') {
        ui->pageLineEdit->setText( arg1.mid(1));
        return;
    }
    
    int totalPageNum = ui->pageLabel->text().remove("/").toInt();
    
    if( arg1.toInt() > totalPageNum) {
        ui->pageLineEdit->setText( QString::number( totalPageNum));
        return;
    }
}
void AccountPage::setAssetType() {
	QString curText = ui->label_12->text();
	int sep = curText.indexOf('(');
	if (sep > 0) {
		ui->label_12->setText(curText.left(sep + 1) + DataMgr::getCurrCurrencyName() + ")");
	}

	curText = ui->label_14->text();
	sep = curText.indexOf('(');
	if (sep > 0) {
		ui->label_14->setText(curText.left(sep + 1) + DataMgr::getCurrCurrencyName() + ")");
	}
}
