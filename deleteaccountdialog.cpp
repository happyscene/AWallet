#include "deleteaccountdialog.h"
#include "ui_deleteaccountdialog.h"
#include "goopal.h"
#include "debug_log.h"
#include "datamgr.h"

#include <QDebug>
#include <QFocusEvent>

#include "waitingpage.h"

DeleteAccountDialog::DeleteAccountDialog(QString name , QWidget *parent) :
    QDialog(parent),
    accountName(name),
    yesOrNo( false),
    ui(new Ui::DeleteAccountDialog),
	waitingPage(nullptr)
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    ui->setupUi(this);

    setParent(Goopal::getInstance()->mainFrame);

    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::FramelessWindowHint);

    ui->widget->setObjectName("widget");
    ui->widget->setStyleSheet("#widget {background-color:rgba(10, 10, 10,100);}");
    ui->containerWidget->setObjectName("containerwidget");
    ui->containerWidget->setStyleSheet("#containerwidget{background-color: rgb(246, 246, 246);border:1px groove rgb(180,180,180);}");

    ui->pwdLineEdit->setStyleSheet("color:black;border:1px solid #CCCCCC;border-radius:3px;");
    ui->pwdLineEdit->setPlaceholderText( tr("Login password"));
    ui->pwdLineEdit->setTextMargins(8,0,0,0);
    ui->pwdLineEdit->setAttribute(Qt::WA_InputMethodEnabled, false);

    ui->okBtn->setStyleSheet("QToolButton{background-color:#469cfc;color:#ffffff;border:0px solid rgb(64,153,255);border-radius:3px;}"
                             "QToolButton:hover{background-color:#62a9f8;}"
                             "QToolButton:disabled{background-color:#cecece;}");
    ui->okBtn->setText(tr("Ok"));

    ui->cancelBtn->setStyleSheet("QToolButton{background-color:#ffffff;color:#282828;border:1px solid #62a9f8;border-radius:3px;}"
                                 "QToolButton:hover{color:#62a9f8;}");
    ui->cancelBtn->setText(tr("Cancel"));

    ui->okBtn->setEnabled(false);

    ui->pwdLineEdit->setFocus();

	connect(DataMgr::getInstance(), &DataMgr::onWalletCheckPassphrase, this, &DeleteAccountDialog::onCheckPasswordCallback);
	connect(DataMgr::getInstance(), &DataMgr::onWalletAccountDelete, this, &DeleteAccountDialog::onAccountDeleteCallback);
	
    DLOG_QT_WALLET_FUNCTION_END;
}

DeleteAccountDialog::~DeleteAccountDialog()
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    delete ui;
    DLOG_QT_WALLET_FUNCTION_END;
}

void DeleteAccountDialog::on_okBtn_clicked()
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;

	const QString& password = ui->pwdLineEdit->text();
	if (password.isEmpty())
		return;

	DataMgr::getInstance()->walletCheckPassphrase(password);

	waitingPage = new WaitingPage(Goopal::getInstance()->mainFrame);
	waitingPage->setAttribute(Qt::WA_DeleteOnClose);
	waitingPage->move(0, 0);
	waitingPage->show();

    DLOG_QT_WALLET_FUNCTION_END;
}

void DeleteAccountDialog::onCheckPasswordCallback(const QString &result)
{
	QJsonParseError json_error;
	QJsonDocument json_doucment = QJsonDocument::fromJson(("{" + result + "}").toLatin1(), &json_error);
	QJsonObject json_object = json_doucment.object();

	QJsonObject::iterator result_itr = json_object.find("result");
	if (result_itr != json_object.end())
	{
		bool ret = result_itr->toBool();
		if (ret)
		{
			DataMgr::getInstance()->walletAccountDelete(accountName);
		}
		else
		{
			if (waitingPage != nullptr)
			{
				delete waitingPage;
				waitingPage = nullptr;
			}

			ui->tipLabel1->setPixmap(QPixmap(DataMgr::getDataMgr()->getWorkPath() + "pic2/wrong.png"));
			ui->tipLabel2->setText("<body><font style=\"font-size:12px\" color=#FF8880>" + tr("Wrong Password!") + "</font></body>");
		}
	}
	else
	{
		if (waitingPage != nullptr)
		{
			delete waitingPage;
			waitingPage = nullptr;
		}

		QJsonObject::iterator error_itr = json_object.find("error");
		if (error_itr != json_object.end())
		{
			const QJsonObject& error_obj = error_itr->toObject();
			const QJsonObject& data_obj = error_obj.value("data").toObject();
			int error_code = data_obj.value("code").toInt();

			if (error_code == 20015)
			{
				ui->tipLabel1->setPixmap(QPixmap(DataMgr::getDataMgr()->getWorkPath() + "pic2/wrong.png"));
				ui->tipLabel2->setText("<body><font style=\"font-size:12px\" color=#FF8880>" + tr("At least 8 letters!") + "</font></body>");
			}
		}
	}
}

void DeleteAccountDialog::onAccountDeleteCallback(const QString &result)
{
	if (waitingPage != nullptr)
	{
		delete waitingPage;
		waitingPage = nullptr;
	}

	QJsonParseError json_error;
	QJsonDocument json_doucment = QJsonDocument::fromJson(("{" + result + "}").toLatin1(), &json_error);
	QJsonObject json_object = json_doucment.object();

	QJsonObject::iterator result_itr = json_object.find("result");
	if (result_itr != json_object.end())
	{
		bool ret = result_itr->toBool();
		if (ret)
		{
			DataMgr::getInstance()->deleteAccountInfo(accountName);
			yesOrNo = true;
			close();
		}
	}
}

void DeleteAccountDialog::on_cancelBtn_clicked()
{
    yesOrNo = false;
    close();
}

void DeleteAccountDialog::on_pwdLineEdit_textChanged(const QString &arg1)
{
    if( arg1.indexOf(' ') > -1)
    {
        ui->pwdLineEdit->setText( ui->pwdLineEdit->text().remove(' '));
        return;
    }

    if( arg1.isEmpty())
    {
        ui->okBtn->setEnabled(false);
    }
    else
    {
        ui->okBtn->setEnabled(true);
    }

    ui->tipLabel1->setPixmap(QPixmap(""));
    ui->tipLabel2->setText("");
}

bool DeleteAccountDialog::pop()
{
    ui->pwdLineEdit->grabKeyboard();

//    QEventLoop loop;
//    show();
//    connect(this,SIGNAL(accepted()),&loop,SLOT(quit()));
//    loop.exec();  //进入事件 循环处理，阻塞

    move(0,0);
    exec();

    return yesOrNo;
}

void DeleteAccountDialog::on_pwdLineEdit_returnPressed()
{
    on_okBtn_clicked();
}
