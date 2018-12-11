#include "rightclickmenudialog.h"
#include "ui_rightclickmenudialog.h"
#include "../goopal.h"
#include "datamgr.h"
#include "commondialog.h"

#include <QDebug>

RightClickMenuDialog::RightClickMenuDialog( QString name, QWidget *parent) :
    QMenu(parent),
    accountName(name)
{
	transferAction = new QAction(tr("Start transfer"), this);
	connect(transferAction, SIGNAL(triggered()), this, SLOT(on_transferBtn_clicked()));
	addAction(transferAction);

	//renameAction = new QAction(tr("Edit account name"), this);
	//connect(renameAction, SIGNAL(triggered()), this, SLOT(on_renameBtn_clicked()));

	//deleteAction = new QAction(tr("Delete account"), this);
	//connect(deleteAction, SIGNAL(triggered()), this, SLOT(on_deleteBtn_clicked()));
	//addAction(deleteAction);
}

RightClickMenuDialog::~RightClickMenuDialog()
{
	if (transferAction != nullptr)
		delete transferAction;
}

void RightClickMenuDialog::on_transferBtn_clicked()
{
	bool updated = DataMgr::getInstance()->isUpdate();

	if (!updated) {
		close();
		CommonDialog commonDialog(CommonDialog::OkOnly);
		commonDialog.setText(tr("Please wait for wallet to sync"));
		commonDialog.pop();
	} else {
		close();
		emit transferFromAccount(accountName);
	}
}

void RightClickMenuDialog::on_renameBtn_clicked()
{
    close();
    emit renameAccount(accountName);
}

void RightClickMenuDialog::on_deleteBtn_clicked()
{
    close();
    emit deleteAccount(accountName);
}