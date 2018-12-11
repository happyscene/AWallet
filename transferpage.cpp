#include <QDebug>
#include <QPainter>
#include <QTextCodec>
#include <QDir>

#include <QClipboard>

#ifdef WIN32 
#include <windows.h>
#endif

#include "datamgr.h"
#include "rpcmgr.h"

#include "transferpage.h"
#include "ui_transferpage.h"
#include "debug_log.h"
#include "gop_common_define.h"
#include "contactdialog.h"
#include "remarkdialog.h"
#include "commondialog.h"
#include "transferconfirmdialog.h"
#include "waitingpage.h"
#include "goopal.h"

TransferPage::TransferPage(QString name,QWidget *parent) :
    QWidget(parent),
    accountName(name),
    inited(false),
    transfer_time(0),
    waitingPage(nullptr),
    ui(new Ui::TransferPage)
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;

    ui->setupUi(this);
	
    setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Background, QColor(255,255,255));
    setPalette(palette);

	QString language = DataMgr::getInstance()->getSettings()->value("/settings/language").toString();
    if( language.isEmpty() || language == "English")
    {
        ui->addContactBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/contactBtn2_En.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}"
                                         "QToolButton:hover{background-image:url(%2pic2/contactBtn2_hover_En.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()).arg(DataMgr::getDataMgr()->getWorkPath()));
        //ui->tipLabel2->setPixmap(QPixmap(DataMgr::getDataMgr()->getWorkPath() + "pic2/feeTip_En.png"));
        //ui->tipLabel2->setGeometry(425,260,229,96);

        delegateLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/delegateLabel_En.png";
        registeredLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/registeredLabel_En.png";
    }
    else
    {
        ui->addContactBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/contactBtn2.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}"
                                         "QToolButton:hover{background-image:url(%2pic2/contactBtn2_hover.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()).arg(DataMgr::getDataMgr()->getWorkPath()));
        //ui->tipLabel2->setPixmap(QPixmap(DataMgr::getDataMgr()->getWorkPath() + "pic2/feeTip.png"));
        //ui->tipLabel2->setGeometry(425,260,317,77);

        delegateLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/delegateLabel.png";
        registeredLabelString = DataMgr::getDataMgr()->getWorkPath() + "pic2/registeredLabel.png";
    }

    if( accountName.isEmpty())  // 如果是点击账单跳转
    {
        if( DataMgr::getInstance()->getAccountInfo()->size() > 0) {
            accountName = DataMgr::getInstance()->getAccountInfo()->keys().at(0);
        } else {
			// 如果还没有账户
            emit back();    // 跳转在functionbar  这里并没有用
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

	QString address = DataMgr::getInstance()->getAccountInfo()->value(accountName).address;
    ui->addressLabel->setText(address);

//  bool voteOrNot = Goopal::getInstance()->configFile->value("/settings/voteOrNot").toBool();

//  if( Goopal::getInstance()->jsonDataValue("id_wallet_get_all_approved_accounts").contains("\"name\":\"") && voteOrNot)
	if (false) {
        ui->voteCheckBox->setChecked( true);
    } else {
        ui->voteCheckBox->setChecked( false);
    }

    ui->amountLineEdit->setStyleSheet("color:black;border:1px solid #CCCCCC;border-radius:3px;");
    ui->amountLineEdit->setTextMargins(8,0,0,0);

    QRegExp rx1("^([0]|[1-9][0-9]{0,10})(?:\\.\\d{1,5})?$|(^\\t?$)");
    QRegExpValidator *pReg1 = new QRegExpValidator(rx1, this);
    ui->amountLineEdit->setValidator(pReg1);
    ui->amountLineEdit->setAttribute(Qt::WA_InputMethodEnabled, false);

    ui->sendtoLineEdit->setStyleSheet("color:black;border:1px solid #CCCCCC;border-radius:3px;");
    ui->sendtoLineEdit->setTextMargins(8,0,0,0);
    QRegExp regx("[a-zA-Z0-9\-\.\ \n]+$");
    QValidator *validator = new QRegExpValidator(regx, this);
    ui->sendtoLineEdit->setValidator( validator );
    ui->sendtoLineEdit->setAttribute(Qt::WA_InputMethodEnabled, false);

    ui->feeLineEdit->setStyleSheet("color:black;border:1px solid #CCCCCC;border-radius:3px;");
    ui->feeLineEdit->setTextMargins(8,0,0,0);
    ui->feeLineEdit->setAttribute(Qt::WA_InputMethodEnabled, false);

    QRegExp rx("^([0]|[1-9][0-9]{0,5})(?:\\.\\d{1,2})?$|(^\\t?$)");
    QRegExpValidator *pReg = new QRegExpValidator(rx, this);
    ui->feeLineEdit->setValidator(pReg);

    ui->messageLineEdit->setStyleSheet("color:black;border:1px solid #CCCCCC;border-radius:3px;");
    ui->messageLineEdit->setTextMargins(8,0,0,0);

    //ui->tipLabel2->hide();
    ui->tipLabel2->setWordWrap(true);
    ui->tipLabel3->hide();
    ui->tipLabel4->hide();
    ui->tipLabel5->hide();
    ui->tipLabel6->hide();

    ui->tipLabel5->setPixmap(QPixmap(DataMgr::getDataMgr()->getWorkPath() + "pic2/wrong.png"));

    on_amountLineEdit_textChanged(ui->amountLineEdit->text());

    ui->sendBtn->setStyleSheet("QToolButton{background-color:#469cfc;color:#ffffff;border:0px solid rgb(64,153,255);border-radius:3px;}"
                               "QToolButton:hover{background-color:#62a9f8;}"
                               "QToolButton:disabled{background-color:#cecece;}");
    ui->sendBtn->setText(tr("Transfer"));

#ifdef WIN32
    ui->accountComboBox->setStyleSheet(QString("QComboBox {border: 1px solid gray;border-radius: 3px;padding: 1px 2px 1px 8px;min-width: 9em;}"
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
#endif // WIN32 //zxlrun
    ui->voteCheckBox->setStyleSheet(QString("QCheckBox::indicator{ image:url(%1pic2/checkBox_unchecked.png); }"
                                    "QCheckBox::indicator:checked{ image:url(%2pic2/checkBox_checked.png); }").arg(DataMgr::getDataMgr()->getWorkPath()).arg(DataMgr::getDataMgr()->getWorkPath()));

    ui->infoBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/info.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()));
    ui->infoBtn->installEventFilter( this);

    ui->copyBtn->setStyleSheet(QString("QToolButton{background-image:url(%1pic2/copyBtn.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}"
        "QToolButton:hover{background-image:url(%2pic2/copyBtn_hover.png);background-repeat: repeat-xy;background-position: center;background-attachment: fixed;background-clip: padding;border-style: flat;}").arg(DataMgr::getDataMgr()->getWorkPath()).arg(DataMgr::getDataMgr()->getWorkPath()));
#ifdef WIN32
    ui->copyBtn->move(ui->addressLabel->x() + ui->addressLabel->text().length() * 10 + 18, 120);
#else
    ui->copyBtn->move(ui->addressLabel->x() + ui->addressLabel->text().length() * 7.5 + 18, 120);
#endif // WIN32 //zxlrun
    ui->copyBtn->setToolTip(tr("copy to clipboard"));

    getContactsList();

    ui->delegateLabel2->adjustSize();
    ui->changeAccountLabel->move(ui->delegateLabel2->x() + ui->delegateLabel2->width() + 2, 42);
	ui->accountComboBox->move(ui->changeAccountLabel->x() + ui->changeAccountLabel->width() + 2, 46);
    ui->identityLabel->move( ui->accountComboBox->x() + ui->accountComboBox->width() + 2, 41);

	inited = true;

	showBalance();
    setFeeHint();

	ui->infoBtn->hide();
	ui->voteCheckBox->hide();
	ui->addContactBtn->hide();

	connect(DataMgr::getInstance(), &DataMgr::onWalletTransferToAddressWithId, this, &TransferPage::walletTransferToAddress);
	connect(DataMgr::getInstance(), &DataMgr::onWalletTransferToPublicAccountWithId, this, &TransferPage::walletTransferToAddress);
	connect(DataMgr::getDataMgr(), &DataMgr::onCallContract, this, &TransferPage::tokenTransferTo);
	connect(DataMgr::getInstance(), &DataMgr::onWalletCheckAddress, this, &TransferPage::walletCheckAddress);

	refresh();

    DLOG_QT_WALLET_FUNCTION_END;
}

void TransferPage::setbalanceLabel(QString balance, QString token)
{
    ui->balanceLabel->setText( "<body><font style=\"font-size:20px\" color=#000000>" + balance + "</font><font style=\"font-size:12px\" color=#000000>" +token+ "</font></body>" );
    int height = ui->balanceLabel->geometry().height();
	ui->balanceLabel->adjustSize();
	QRect labelGeometry = ui->balanceLabel->geometry();
	labelGeometry.setHeight(height);
	ui->balanceLabel->setGeometry(labelGeometry);

    ui->balanceLabel->move(555,44);
    ui->balanceLabel2->move(553 - ui->balanceLabel2->width(), 42);
}

TransferPage::~TransferPage()
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;

    if(waitingPage != nullptr)
    {
        delete waitingPage;
        waitingPage = nullptr;
    }

    delete ui;

    DLOG_QT_WALLET_FUNCTION_END;
}

void TransferPage::walletCheckAddress(QString result)
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;

    if(waitingPage != nullptr)
    {
        delete waitingPage;
        waitingPage = nullptr;
    }

	result = result.mid(result.indexOf(":") + 1, result.length());

	if (result == "true")
	{
		QString amount_text(ui->amountLineEdit->text());
		QString account_text(ui->sendtoLineEdit->text());
		QString fee_text(ui->feeLineEdit->text());

		if (amount_text.toDouble() > -0.0000001 && amount_text.toDouble() < 0.0000001) {
			CommonDialog tipDialog(CommonDialog::OkOnly);
			tipDialog.setText(tr("The amount can not be 0"));
			tipDialog.pop();
			return;
		}

		if (fee_text.toDouble() > -0.0000001 && fee_text.toDouble() < 0.0000001) {
			CommonDialog tipDialog(CommonDialog::OkOnly);
			tipDialog.setText(tr("The fee can not be 0"));
			tipDialog.pop();
			return;
		}

		QString strBalanceTemp = DataMgr::getInstance()->getAccountInfo()->value(accountName).getBalance(COMMONASSET);
		double dBalance = strBalanceTemp.toDouble();
        if (dBalance < 0.1) {
            CommonDialog tipDialog(CommonDialog::OkOnly);
            tipDialog.setText(tr("The ACT amount must be greater than 0.1 ."));
            tipDialog.pop();
            return;
        }

		QString remark = ui->messageLineEdit->text();
		auto currencyInfo = DataMgr::getCurrentCurrency();
		if (currencyInfo.isAsset() == false) {
			remark.replace('|', '+');
		}
		//    remark.remove(' ');
		if (remark.size() == 0) {
			// 转地址如果没有备注 会自动添加 TO GOPDD...   所以添加空格
			remark = " ";
		}

		QTextCodec* utfCodec = QTextCodec::codecForName("UTF-8");
		QByteArray ba = utfCodec->fromUnicode(remark);
		if (ba.size() > 40) {
			CommonDialog tipDialog(CommonDialog::OkOnly);
			tipDialog.setText(tr("Message length more than 40 bytes!"));
			tipDialog.pop();
			return;
		}

		TransferConfirmDialog transferConfirmDialog(account_text, amount_text, fee_text, remark);
		bool yOrN = transferConfirmDialog.pop();
		if (yOrN) {
			auto currencyInfo = DataMgr::getCurrentCurrency();
			if (currencyInfo.isAsset()) {
				QString str = "wallet_set_transaction_fee " + fee_text + '\n';

				Misc::write(str, RpcMgr::getInstance()->currentProcess());
				QString result = Misc::read(RpcMgr::getInstance()->currentProcess());

				QString vote = ui->voteCheckBox->isChecked() ? "vote_all" : "vote_none";

				if (account_text.mid(0, 3) == "ACT") {
					DataMgr::getInstance()->walletTransferToAddressWithId(accountName, amount_text, currencyInfo.name, accountName, account_text, remark, vote);
					qDebug() << "id_wallet_transfer_to_public_account_" + accountName;
				}
				else {
					DataMgr::getInstance()->walletTransferToPublicAccountWithId(accountName, amount_text, currencyInfo.name, accountName, account_text, remark, vote);
                    qDebug() << "id_wallet_transfer_to_public_account_" + accountName;
				}
			}
			else {
				DataMgr::getInstance()->tokenTransferTo(accountName, currencyInfo.contractId, account_text, amount_text.toDouble(), fee_text.toDouble(), remark);
			}
		}
	}
	else
	{
		CommonDialog tipDialog(CommonDialog::OkOnly);
		tipDialog.setText(tr("Wrong address!"));
		tipDialog.pop();
	}
    DLOG_QT_WALLET_FUNCTION_END;
}

void TransferPage::on_accountComboBox_currentIndexChanged(const QString &arg1)
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;

    if( !inited)  return;

    accountName = arg1;

	QString address = DataMgr::getInstance()->getAccountInfo()->value(accountName).address;
    ui->addressLabel->setText(address);

	showBalance();
    on_amountLineEdit_textChanged(ui->amountLineEdit->text());

	DataMgr::getInstance()->setCurrentAccount(accountName);

	/*
    // 如果是已注册账户
    if( Goopal::getInstance()->registerMapValue(accountName) != "NO"
            && !Goopal::getInstance()->registerMapValue(accountName).isEmpty())
    {
        // 如果是代理
        if( Goopal::getInstance()->delegateAccountList.indexOf(accountName) != -1)
        {
            ui->identityLabel->setPixmap(QPixmap(delegateLabelString));

//            Goopal::getInstance()->postRPC( toJsonFormat( "id_wallet_get_delegate_statue_" + accountName, "wallet_get_delegate_statue", QStringList() << accountName ));
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

    DLOG_QT_WALLET_FUNCTION_END;
}

void TransferPage::on_sendBtn_clicked()
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;

	/*
	int i = 10000;
	while (i--)
	DataMgr::getInstance()->callContract(QString("CON6t6b4kKBVSrXVh5HyH1oYNS8Tz8fxUMjc"), QString("asz"), 
		QString("transfer_to"), QString("ACTFc5feSCYpn5mUQZvp5vZYhsF3F6wxagz5|500|dsdfgsdafsdf"), QString("ACT"), 1);
	*/

    if(ui->sendtoLineEdit->text().size() == 0) {      
        CommonDialog tipDialog(CommonDialog::OkOnly);
        tipDialog.setText( tr("Input Address."));
        tipDialog.pop();
        return;
    }

    if (ui->amountLineEdit->text().size() == 0) {
        CommonDialog tipDialog(CommonDialog::OkOnly);
        tipDialog.setText(tr("Please enter the amount."));
        tipDialog.pop();
        return;
    }

	//验证地址有效性
	QString address = ui->sendtoLineEdit->text();
	DataMgr::getInstance()->walletCheckAddress(address);

    waitingPage = new WaitingPage(Goopal::getInstance()->mainFrame);
    waitingPage->setAttribute(Qt::WA_DeleteOnClose);
    waitingPage->move(0, 0);
    waitingPage->show();

    DLOG_QT_WALLET_FUNCTION_END;
}

void TransferPage::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
    painter.setPen(QPen(QColor(177, 183, 188), Qt::SolidLine));
    painter.setBrush(QBrush(QColor(229, 229, 229), Qt::SolidPattern));
    painter.drawRect(-1, -1, 828, 88);

}

void TransferPage::on_amountLineEdit_textChanged(const QString &arg1)
{
	auto currencyInfo = DataMgr::getCurrentCurrency();
	if (currencyInfo.isAsset()) {
		double amount = ui->amountLineEdit->text().toDouble();
		double fee = ui->feeLineEdit->text().toDouble();

		QString strBalanceTemp = DataMgr::getInstance()->getAccountInfo()->value(accountName).getBalance(currencyInfo.name);
		double dBalance = strBalanceTemp.remove(",").toDouble();

		if (currencyInfo.name == COMMONASSET && amount + fee > dBalance + 0.000001) {
			ui->tipLabel3->show();
			ui->tipLabel5->show();
			ui->sendBtn->setEnabled(false);
		}
		else if (amount > dBalance) {
			ui->tipLabel3->show();
			ui->tipLabel5->show();
			ui->sendBtn->setEnabled(false);
		}
		else {
			ui->tipLabel3->hide();
			ui->tipLabel5->hide();
			ui->sendBtn->setEnabled(true);
		}
	} else {
		double amount = ui->amountLineEdit->text().toDouble();
		double balance = DataMgr::getInstance()->getAccountTokenBalance(accountName, currencyInfo.name).toDouble();
		if (amount > balance) {
			ui->tipLabel3->show();
			ui->tipLabel5->show();
			ui->sendBtn->setEnabled(false);
		} else {
			ui->tipLabel3->hide();
			ui->tipLabel5->hide();
			ui->sendBtn->setEnabled(true);
		}
	}
}

void TransferPage::on_sendtoLineEdit_textChanged(const QString &arg1)
{
    if( ui->sendtoLineEdit->text().contains(" ") || ui->sendtoLineEdit->text().contains("\n")) { 
		// 不判断就remove的话 右键菜单撤销看起来等于不能用
        ui->sendtoLineEdit->setText( ui->sendtoLineEdit->text().simplified().remove(" "));
    }

//    ui->sendtoLineEdit->setText( ui->sendtoLineEdit->text().remove("\n"));
    if( ui->sendtoLineEdit->text().isEmpty() || ui->sendtoLineEdit->text().mid(0,3) == "ACT") {
        ui->tipLabel4->hide();
        return;
    }

    if( ui->sendtoLineEdit->text().toInt() == 0)   // 不能是纯数字
    {
		//Goopal::getInstance()->postRPC( toJsonFormat( "id_blockchain_get_account_" + ui->sendtoLineEdit->text(), "blockchain_get_account", QStringList() << ui->sendtoLineEdit->text() ));
    } else {
        ui->tipLabel4->setText(tr("Invalid add."));
        ui->tipLabel4->show();
    }

}

void TransferPage::refresh()
{
    qDebug() << "transferPage: refresh begin";
    showBalance();
    setAssetType();
    setFeeHint();
    qDebug() << "transferPage: refresh end";
}

void TransferPage::tokenTransferTo(QString result)
{
	if (result.mid(0, 18) == "\"result\":{\"index\":") {
		CommonDialog tipDialog(CommonDialog::OkOnly);
		tipDialog.setText(tr("The transfer request has been submitted."));
		tipDialog.pop();
		emit showAccountPage(accountName);
	} else {

	}
}

void TransferPage::on_addContactBtn_clicked()
{
    ContactDialog contactDialog(this);
    connect(&contactDialog,SIGNAL(contactSelected(QString)), this, SLOT(contactSelected(QString)));
    contactDialog.move( ui->addContactBtn->mapToGlobal( QPoint(0,0)));
    contactDialog.exec();
}

void TransferPage::contactSelected(QString contact)
{
    ui->sendtoLineEdit->setText(contact);
}


void TransferPage::getContactsList()
{
//    if( !Goopal::getInstance()->contactsFile->open(QIODevice::ReadOnly))
	if (false)
    {
        qDebug() << "contact.dat not exist";
        contactsList.clear();
        return;
    }
	/*
    QString str = QByteArray::fromBase64( Goopal::getInstance()->contactsFile->readAll());
    QStringList strList = str.split(";");
    strList.removeLast();
    int size = strList.size();

    for( int i = 0; i < size; i++)
    {
        QString str2 = strList.at(i);
        contactsList += str2.left( str2.indexOf("="));
    }
	*/

  //  Goopal::getInstance()->contactsFile->close();
}

QString TransferPage::getCurrentAccount()
{
    return accountName;
}

void TransferPage::on_voteCheckBox_clicked()
{
    if( ui->voteCheckBox->isChecked()) {
        //if( !Goopal::getInstance()->jsonDataValue("id_wallet_get_all_approved_accounts").contains("\"name\":\"") )
		if (false) {
            CommonDialog tipDialog(CommonDialog::OkOnly);
            tipDialog.setText(tr("Have not set a favorite delegate, set it in delegate page"));
            tipDialog.pop();
            ui->voteCheckBox->setChecked(false);
        }
    }
}

bool TransferPage::eventFilter(QObject *watched, QEvent *e)
{
    if( watched == ui->infoBtn) {
        if( e->type() == QEvent::Leave) {
            //ui->tipLabel2->hide();
            return true;
        }
    }

    return QWidget::eventFilter(watched,e);
}

void TransferPage::on_infoBtn_clicked()
{
    ui->tipLabel2->show();
}

void TransferPage::setAddress(QString address)
{
    ui->sendtoLineEdit->setText(address);
}

void TransferPage::walletTransferToAddress(QString id, QString result)
{
	if (id == "id_wallet_transfer_to_address_" + accountName
		|| id == "id_wallet_transfer_to_public_account_" + accountName) {
		qDebug() << id << result;
		if (result.mid(0, 18) == "\"result\":{\"index\":") {
			// record_id 被改为了 entry_id
			QString recordId = result.mid(result.indexOf("\"entry_id\"") + 12, 40);

			/*
			if( !Goopal::getInstance()->pendingFile->open(QIODevice::ReadWrite))
			{
			qDebug() << "pending.dat not exist";
			return;
			}

			QByteArray ba = QByteArray::fromBase64( Goopal::getInstance()->pendingFile->readAll());
			ba += QString( recordId + "," + accountName + "," + ui->amountLineEdit->text() + "," + ui->feeLineEdit->text() + ";").toUtf8();
			ba = ba.toBase64();
			Goopal::getInstance()->pendingFile->resize(0);
			QTextStream ts(Goopal::getInstance()->pendingFile);
			ts << ba;

			Goopal::getInstance()->pendingFile->close();
			*/

			CommonDialog tipDialog(CommonDialog::OkOnly);
			tipDialog.setText(tr("The transfer request has been submitted."));
			tipDialog.pop();

			emit showAccountPage(accountName);
		} else {

			int pos = result.indexOf("\"message\":\"") + 11;
			QString errorMessage = result.mid(pos, result.indexOf("\"", pos) - pos);
			qDebug() << "errorMessage : " << errorMessage;

			if (errorMessage == "Assert Exception") {
				if (result.contains("\"format\":\"my->is_receive_account( from_account_name ): Invalid account name\",")) {
					CommonDialog tipDialog(CommonDialog::OkOnly);
					tipDialog.setText(tr("This name has been registered, please rename this account!"));
					tipDialog.pop();
				} else {
					CommonDialog tipDialog(CommonDialog::OkOnly);
					tipDialog.setText(tr("Wrong address!"));
					tipDialog.pop();
				}

			} else if (errorMessage == "imessage size bigger than soft_max_lenth") {
				CommonDialog tipDialog(CommonDialog::OkOnly);
				tipDialog.setText(tr("Message too long!"));
				tipDialog.pop();

			} else if (errorMessage == "invalid transaction expiration") {
				CommonDialog tipDialog(CommonDialog::OkOnly);
				tipDialog.setText(tr("Failed: You need to wait for synchronization to complete"));
				tipDialog.pop();
			} else if (errorMessage == "insufficient funds") {
				CommonDialog tipDialog(CommonDialog::OkOnly);
				tipDialog.setText(tr("Not enough ACTs!"));
				tipDialog.pop();
			} else if (errorMessage == "Out of Range") {
				CommonDialog tipDialog(CommonDialog::OkOnly);
				tipDialog.setText(tr("Wrong address!"));
				tipDialog.pop();
			} else if (errorMessage == "Parse Error") {
				CommonDialog tipDialog(CommonDialog::OkOnly);
				tipDialog.setText(tr("Wrong address!"));
				tipDialog.pop();
			} else {
				CommonDialog tipDialog(CommonDialog::OkOnly);
				tipDialog.setText(tr("Transaction sent failed"));
				tipDialog.pop();
			}
		}
	}
}

void TransferPage::on_feeLineEdit_textChanged(const QString &arg1)
{
    on_amountLineEdit_textChanged("");
}

void TransferPage::on_messageLineEdit_textChanged(const QString &arg1)
{
    QTextCodec* utfCodec = QTextCodec::codecForName("UTF-8");
    QByteArray ba = utfCodec->fromUnicode(arg1);
    if( ba.size() > 40) {
        ui->tipLabel6->show();
    } else {
        ui->tipLabel6->hide();
    }
}

void TransferPage::showBalance()
{
	QString balance;
	auto currencyInfo = DataMgr::getCurrentCurrency();
	if (currencyInfo.isAsset())
		balance = DataMgr::getInstance()->getAccountInfo()->value(accountName).getBalance(currencyInfo.name);
	else
		balance = DataMgr::getInstance()->getAccountTokenBalance(accountName, currencyInfo.name);

    if (!balance.isEmpty()){
		setbalanceLabel(checkZero(balance.toDouble()), " " + DataMgr::getCurrCurrencyName());
    } else {
		setbalanceLabel("0", " " + DataMgr::getCurrCurrencyName());
    }
}

void TransferPage::on_copyBtn_clicked()
{
    QClipboard* clipBoard = QApplication::clipboard();
    clipBoard->setText(ui->addressLabel->text());

    CommonDialog commonDialog(CommonDialog::OkOnly);
    commonDialog.setText(tr("Copy to clipboard"));
    commonDialog.pop();
}

void TransferPage::setAssetType()
{
    ui->unitLabel->setText(DataMgr::getCurrCurrencyName());
}

void TransferPage::setFeeHint()
{
	auto currencyInfo = DataMgr::getCurrentCurrency();
	if (currencyInfo.isAsset() == false) {
        ui->tipLabel2->setText(tr("Poundage is estimated, please refer to the actual occurrence."));
    } else {
        ui->tipLabel2->setText("");
    }
}
