#include <QPainter>
#include <QLayout>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QDebug>
#include <QMenu>
#include <QApplication>

#include "firstlogin.h"
#include "normallogin.h"
#include "mainpage.h"
#include "accountpage.h"
#include "transferpage.h"
#include "bottombar.h"
#include "lockpage.h"
#include "titlebar.h"
#include "frame.h"
#include "goopal.h"
#include "debug_log.h"
#include "waitingforsync.h"
#include "functionbar.h"
#include "contactpage.h"
#include "upgradepage.h"
#include "applydelegatepage.h"
#include "selectgoppathwidget.h"
#include "control/shadowwidget.h"
#include "control/showbottombarwidget.h"

#include "datamgr.h"
#include "rpcmgr.h"

#ifdef WIN32
    #include "qt_windows.h"
#endif //zxlwin
#include <qmessagebox.h>


#define TRX_LOG   "TOKEN:SMC:TRANSLOG:201611111111:asd:afd:10123:END"
#define BALANCE_1 "TOKEN:SMC:BLC:asd:0:END"
#define BALANCE_2 "TOKEN:SMC:BLC:asd:0:END"

#define UPDATE_TOOL_NAME "AchainUp.exe"


Frame::Frame() : 
    timer(NULL),
	timerUpdate(NULL),
    mainPage(NULL),
    accountPage(NULL),
    transferPage(NULL),
    bottomBar(NULL),
    centralWidget(NULL),
    lockPage(NULL),
    titleBar(NULL),
    contactPage(NULL),
    upgradePage(NULL),
    applyDelegatePage(NULL),
    functionBar(NULL),
    shadowWidget(NULL),
    timerForAutoRefresh(NULL),
    showBottomBarWidget(NULL),
	waitingForSync(nullptr)
{
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    setWindowFlags(Qt::Window|Qt::FramelessWindowHint |Qt::WindowSystemMenuHint|Qt::WindowMinimizeButtonHint|Qt::WindowMaximizeButtonHint);
#ifdef WIN32
    ::SystemParametersInfo(SPI_GETWORKAREA, 0, &this->rtConfined, 0);     //zxlwin
    ::GetWindowRect(::GetDesktopWindow(), &this->rtDefault);   //zxlwin
#endif // WIN32
    setFrameShape(QFrame::NoFrame);
    //  setMouseTracking(true);
    mouse_press = false;
    currentPageNum = 0;
    lastPage = "";
    currentAccount = "";
    //Get default config language
    QString language = DataMgr::getInstance()->getSettings()->value("/settings/language").toString();
	if (language.isEmpty()) {
		language = "English";
    }
	setLanguage(language);
    DataMgr::getInstance()->setLanguage(language);

    setGeometry(240, 110, 960, 580);
    shadowWidget = new ShadowWidget(this);
    shadowWidget->init(this->size());
    //  如果是第一次使用(未设置 blockchain 路径)
    connect(DataMgr::getDataMgr(), &DataMgr::onLock, this, &Frame::walletLock);
    connect(DataMgr::getDataMgr(), &DataMgr::onWalletListMyAddresses, this, &Frame::walletListAccounts);
    connect(DataMgr::getDataMgr(), &DataMgr::onWalletOpen, this, &Frame::openWallet);
    //selectGopPathWidget 启动
    connect(DataMgr::getDataMgr(), &DataMgr::onCallContractTesting, this, &Frame::tokenQueryBalance);
    connect(DataMgr::getDataMgr(), &DataMgr::onCallContract, this, &Frame::tokenTransferTo);
    connect(DataMgr::getInstance(), &DataMgr::onAssetChange, this, &Frame::refresh);
    connect(DataMgr::getDataMgr(), &DataMgr::onWalletAccountBalance, this, &Frame::walletAccountBalance);
	connect(DataMgr::getDataMgr(), &DataMgr::onBalance, this, &Frame::walletAccountBalance);

    if (!DataMgr::getDataMgr()->getSettings()->contains("/settings/achainPath")) {
        showSelectAchainPathWidget();
        
    } else {
        QString path = DataMgr::getDataMgr()->getSettings()->value("/settings/achainPath").toString();
        QDir dir(path);
        QDir dir2(path + "/wallets");
        QDir dir3(path + "/chain");
        
        if (!dir.exists() || (!dir2.exists() && !dir3.exists())) { // 如果数据文件被删除了
            showSelectAchainPathWidget();
            QSettings* config = DataMgr::getDataMgr()->getSettings();
            config->clear();
			config->setValue("/settings/language", language);
            config->setValue("/settings/lockMinutes", 5);
            config->setValue("/settings/notAutoLock", false);
            config->setValue("/settings/showPrivateTips", true);
            DataMgr::getInstance()->lockMinutes = 5;
            DataMgr::getInstance()->notProduce = true;
            DataMgr::getInstance()->showPrivateTips = true;
            /*
            Goopal::getInstance()->configFile->setValue("/settings/minimizeToTray",false);
            Goopal::getInstance()->configFile->setValue("/settings/closeToMinimize",false);
            Goopal::getInstance()->minimizeToTray  = false;
            Goopal::getInstance()->closeToMinimize = false;
            */
            
        } else {
            showWaittingForSyncWidget();
            QSettings* config = DataMgr::getDataMgr()->getSettings();
            DataMgr::getInstance()->lockMinutes = config->value("/settings/lockMinutes").toInt();
            DataMgr::getInstance()->notProduce = config->value("/settings/notAutoLock").toBool();
            
            if (config->value("/settings/showPrivateTips").isNull()) {
                config->setValue("/settings/showPrivateTips", true);
                config->sync();
            }
            
            DataMgr::getInstance()->showPrivateTips = config->value("/settings/showPrivateTips").toBool();

			RpcMgr::getInstance()->startBlockChain();
        }
    }

    trayIcon = new QSystemTrayIcon(this);
    //放在托盘提示信息、托盘图标
    trayIcon ->setToolTip(QString("Achain Wallet ") + ACHAIN_WALLET_VERSION);
    trayIcon ->setIcon(QIcon(DataMgr::getDataMgr()->getWorkPath() + "pic2/achain.ico"));
    //点击托盘执行的事件
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconIsActived(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
    createTrayIconActions();
    createTrayIcon();
    trayIconMenuEmpty = new QMenu(this);//zxlrun
    setStyleSheet("Frame{background-color:white; border: 4px solid #CCCCCC;border-radius:5px;}"
                  "QScrollBar:vertical"
                  "{"
                  "width:8px;"
                  "background:rgba(130,137,143,20%);"
                  "margin:0px,0px,0px,0px;"
                  "padding-top:2px;"
                  "padding-bottom:3px;"
                  "}"
                  "QScrollBar::handle:vertical"
                  "{"
                  "width:8px;"
                  "background:rgba(130,137,143,20%);"
                  " border-radius:4px;"
                  "min-height:20;"
                  "}"
                  "QScrollBar::handle:vertical:hover"
                  "{"
                  "width:8px;"
                  "background:rgba(130,137,143,100%);"
                  " border-radius:4px;"
                  "min-height:20;"
                  "}"
                  "QScrollBar::add-line:vertical"
                  "{"
                  "height:9px;width:8px;"
                  "border-image:url(:/images/a/3.png);"
                  "subcontrol-position:bottom;"
                  "}"
                  "QScrollBar::sub-line:vertical"
                  "{"
                  "height:9px;width:8px;"
                  "border-image:url(:/images/a/1.png);"
                  "subcontrol-position:top;"
                  "}"
                  "QScrollBar::add-line:vertical:hover"
                  "{"
                  "height:9px;width:8px;"
                  "border-image:url(:/images/a/4.png);"
                  "subcontrol-position:bottom;"
                  "}"
                  "QScrollBar::sub-line:vertical:hover"
                  "{"
                  "height:9px;width:8px;"
                  "border-image:url(:/images/a/2.png);"
                  "subcontrol-position:top;"
                  "}"
                  "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical"
                  "{"
                  "background:rgba(0,0,0,0%);"
                  "border-radius:4px;"
                  "}"
                 );
    runUpdate();

    DLOG_QT_WALLET_FUNCTION_END;
}

Frame::~Frame() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    qDebug() << "~Frame begin;";
    
    if (trayIcon) {
        delete trayIcon;
        trayIcon;
    }
    
    if(centralWidget) {
        delete centralWidget;
        centralWidget = NULL;
    }
    
    if (titleBar) {
        delete titleBar;
        titleBar = NULL;
    }
    
    if (timer) {
        delete timer;
        timer = NULL;
    }
    
    if (timerUpdate) {
        timerUpdate->stop();
        delete timerUpdate;
        timerUpdate = NULL;
    }
    
    if (bottomBar) {
        delete bottomBar;
        bottomBar = NULL;
    }
    
    if (lockPage) {
        delete lockPage;
        lockPage = NULL;
    }
    
    if( functionBar) {
        delete functionBar;
        functionBar = NULL;
    }

	if (waitingForSync != nullptr)
	{
		delete waitingForSync;
		waitingForSync = nullptr;
	}
    
    qDebug() << "~Frame end;";
    DLOG_QT_WALLET_FUNCTION_END;
}
//frame init
void Frame::alreadyLogin() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    titleBar = new TitleBar(this);
    titleBar->setGeometry(0, 0, 960, 37);
#ifdef WIN32
    connect(titleBar, SIGNAL(minimum()), this, SLOT(showMinimized()));
#else
    connect(titleBar, SIGNAL(minimum()), this, SLOT(hide()));
#endif // WIN32//zxlrun
    connect(titleBar, SIGNAL(closeGOP()), qApp, SLOT(quit()));
    connect(titleBar, SIGNAL(tray()), this, SLOT(hide()));
    connect(titleBar, SIGNAL(settingSaved()), this, SLOT(settingSaved()));
    connect(titleBar, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(titleBar, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    connect(titleBar, SIGNAL(showAccountPage(QString)), this, SLOT(showAccountPage(QString)));
    connect(titleBar, SIGNAL(resync()), this, SLOT(rebuild()));
    connect(titleBar, SIGNAL(scan()), this, SLOT(scan()));
    titleBar->show();
    centralWidget = new QWidget(this);
    centralWidget->setGeometry(133, 37, 827, 545);
    centralWidget->show();
    bottomBar = new BottomBar(this);
    bottomBar->move(133, 580);
    bottomBar->show();
    bottomBar->dynamicShow();
    connect(bottomBar, SIGNAL(lock()), this, SLOT(showLockPage()));
    connect(bottomBar, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(bottomBar, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    showBottomBarWidget = new ShowBottomBarWidget(centralWidget);
    showBottomBarWidget->setGeometry(0, 525, 827, 20);
    connect(showBottomBarWidget, SIGNAL(showBottomBar()), bottomBar, SLOT(dynamicShow()) );
    showBottomBarWidget->show();
    functionBar = new FunctionBar(this);
    functionBar->move(0, 37);
    functionBar->show();
    connect(functionBar, SIGNAL(showMainPage()), this, SLOT( showMainPage()));
    connect(functionBar, SIGNAL(showAccountPage(QString)), this, SLOT( showAccountPage(QString)));
    connect(functionBar, SIGNAL(showDelegatePage()), this, SLOT( showDelegatePage()));
    connect(functionBar, SIGNAL(showTransferPage()), this, SLOT( showTransferPage()));
    connect(functionBar, SIGNAL(showContactPage()), this, SLOT( showContactPage()));
    connect(functionBar, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(functionBar, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));

    getAccountInfo();

    mainPage = new MainPage(centralWidget);
    mainPage->setAttribute(Qt::WA_DeleteOnClose);
    mainPage->show();
    currentPageNum = 0;
    connect(mainPage, SIGNAL(openAccountPage(QString)), this, SLOT(showAccountPage(QString)));
    connect(mainPage, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(mainPage, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    connect(mainPage, SIGNAL(showUpgradePage(QString)), this, SLOT(showUpgradePage(QString)));
    connect(mainPage, SIGNAL(showApplyDelegatePage(QString)), this, SLOT(showApplyDelegatePage(QString)));
    connect(mainPage, SIGNAL(showTransferPage(QString)), this, SLOT(showTransferPage(QString)));
    connect(mainPage, SIGNAL(newAccount()), this, SLOT(refresh()));

	if (timer != nullptr) {
		timer->stop();
		delete timer;
	}
    timer = new QTimer(this); //  自动锁定
    connect(timer, SIGNAL(timeout()), this, SLOT(autoLock()));
    if(DataMgr::getInstance()->notProduce) {
        timer->start(DataMgr::getInstance()->lockMinutes * 60000);
    }
    
    startTimerForAutoRefresh(); // 自动刷新
    init();

    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::getAccountInfo() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    DataMgr::getDataMgr()->walletListMyAddress();
    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::showLockPage() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    timer->stop();
    shadowWidgetShow();
    DataMgr::getInstance()->lock();
    qDebug() << "lock ";
    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::autoLock() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    timer->stop();
    DataMgr::getInstance()->lock();
    qDebug() << "autolock ";
    DLOG_QT_WALLET_FUNCTION_END;
}
void writeLog(QString log) {
    QFile file("./Achain.log");
    
    //方式：Append为追加，WriteOnly，ReadOnly
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        QMessageBox::critical(NULL, "提示", "无法创建文件");
        return;
    }
    
    QTextStream out(&file);
    out << log << endl;
    out.flush();
    file.close();
}
void Frame::runUpdate() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;

#ifdef WIN32
    QString app_path = QCoreApplication::applicationDirPath();
    //writeLog("app_path " + app_path);
    QString upPath = app_path + "/" + UPDATE_TOOL_NAME;
	QStringList args = QStringList() << "download" << DataMgr::getInstance()->getConfigPath();
	bool result = QProcess::startDetached(upPath, args);
    writeLog(result ? "successed" : "failed");
#endif

    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::autoUpdate() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    runUpdate();
    qDebug() << "timerUpdate ";
    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::unlock() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    
    if( DataMgr::getInstance()->notProduce) {
        timer->start(DataMgr::getInstance()->lockMinutes * 60000);
    }
    
    centralWidget->show();
    bottomBar->dynamicShow();
    titleBar->show();
    qDebug() << "lockPage " << lockPage;
    
    if( lockPage) {
        lockPage->close();
        lockPage = NULL;
    }
    
    qDebug() << "unlock showCurrentDialog";
    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::updateTimer() {
    if( timer != NULL && lockPage == NULL) {
        if( DataMgr::getInstance()->notProduce) {
            timer->stop();
            timer->start(DataMgr::getInstance()->lockMinutes * 60000);
        }
    }
}
void Frame::settingSaved() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    
    if( !DataMgr::getInstance()->notProduce) {
        timer->stop();
    } else {
        timer->start( DataMgr::getInstance()->lockMinutes * 60000);
    }
    
    QString language = DataMgr::getInstance()->getSettings()->value("/settings/language").toString();
    
    if( language.isEmpty()) {
        setLanguage("English");
        
    } else {
        setLanguage(language);
    }
    
    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::privateKeyImported() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    getAccountInfo();
    mainPage->updateAccountList();
    DLOG_QT_WALLET_FUNCTION_END;
}

void Frame::mousePressEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        mouse_press = true;
        //鼠标相对于窗体的位置（或者使用event->globalPos() - this->pos()）
        move_point = event->pos();
    }
    
#ifdef WIN32
    ::ClipCursor(&rtConfined);  //zxlwin
#endif // WIN32
}
void Frame::mouseMoveEvent(QMouseEvent *event) {
    //若鼠标左键被按下
    if(mouse_press) {
        //鼠标相对于屏幕的位置
        QPoint move_pos = event->globalPos();
        //移动主窗体位置
        this->move(move_pos - move_point);
    }
}
void Frame::mouseReleaseEvent(QMouseEvent *) {
    mouse_press = false;
#ifdef WIN32
    ::ClipCursor(&rtDefault);  //zxlwin
#endif // WIN32
}
void Frame::startTimerForAutoRefresh() {
    if( timerForAutoRefresh != NULL) {
        timerForAutoRefresh->stop();
        delete timerForAutoRefresh;
    }
    
    timerForAutoRefresh = new QTimer(this);
    connect(timerForAutoRefresh, SIGNAL(timeout()), this, SLOT(autoRefresh()));
    timerForAutoRefresh->start(AUTO_REFRESH_TIME);
}
void Frame::syncFinished() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
	blockchainStarted();
	DataMgr::getDataMgr()->walletOpen("wallet");
    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::closeCurrentPage() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    qDebug() << " closeCurrentPage :" << currentPageNum;
    
    switch (currentPageNum) {
        case 0:
            mainPage->close();
            mainPage = NULL;
            break;
            
        case 1:
            accountPage->close();
            accountPage = NULL;
            break;
            
        case 2:
            break;
            
        case 3:
            transferPage->close();
            transferPage = NULL;
            break;
            
        case 4:
            contactPage->close();
            contactPage = NULL;
            break;
            
        case 5:
            break;
            
        case 6:
            break;
            
        case 7:
            upgradePage->close();
            upgradePage = NULL;
            break;
            
        case 8:
            applyDelegatePage->close();
            applyDelegatePage = NULL;
            break;
            
        default:
            break;
    }
    
    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::updataView() {
    switch (currentPageNum) {
        case 0:
            mainPage->refresh();
            break;
            
        case 1:
            accountPage->refresh();
            break;
            
        case 2:
            showDelegatePage();
            break;
            
        case 3:
            transferPage->refresh();
            break;
            
        case 4:
            break;
            
        case 5:
            if( lockPage == NULL) {   // 锁定的时候 refresh会崩溃
                accountPage->refresh();
            }
            
            break;
            
        case 6:
            break;
            
        case 7:
            //        showUpgradePage();
            break;
            
        case 8:
            //        showApplyDelegatePage();
            break;
            
        default:
            break;
    }
}
void Frame::refresh() {
    getAccountInfo();
    updataView();
}
void Frame::autoRefresh() {
    getAccountInfo();
    updataView();
    bottomBar->refresh();
}
void Frame::showAccountPage(QString accountName) {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    currentAccount = accountName;
    getAccountInfo();
    closeCurrentPage();
    accountPage = new AccountPage(accountName, centralWidget);
    accountPage->setAttribute(Qt::WA_DeleteOnClose);
    accountPage->show();
    currentPageNum = 1;
    connect(accountPage, SIGNAL(back()), this, SLOT(showMainPage()));
    connect(accountPage, SIGNAL(accountChanged(QString)), this, SLOT(showAccountPage(QString)));
    connect(accountPage, SIGNAL(showUpgradePage(QString)), this, SLOT(showUpgradePage(QString)));
    connect(accountPage, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(accountPage, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    connect(accountPage, SIGNAL(showApplyDelegatePage(QString)), this, SLOT(showApplyDelegatePage(QString)));
    functionBar->choosePage(2);
    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::showTransferPage(QString accountName) {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    closeCurrentPage();
    getAccountInfo();
    transferPage = new TransferPage(accountName, centralWidget);
    transferPage->setAttribute(Qt::WA_DeleteOnClose);
    connect(transferPage, SIGNAL(accountChanged(QString)), this, SLOT(showTransferPage(QString)));
    connect(transferPage, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(transferPage, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    connect(transferPage, SIGNAL(showAccountPage(QString)), this, SLOT(showAccountPage(QString)));
    transferPage->show();
    currentPageNum = 3;
    functionBar->choosePage(3);
    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::refreshCombobox() {
	if (functionBar != nullptr)
	{
		functionBar->refreshResetCombobox();
	}
}
void Frame::showMainPage() {
    closeCurrentPage();
    getAccountInfo();
    mainPage = new MainPage(centralWidget);
    mainPage->setAttribute(Qt::WA_DeleteOnClose);
    mainPage->show();
    currentPageNum = 0;
    connect(mainPage, SIGNAL(openAccountPage(QString)), this, SLOT(showAccountPage(QString)));
    connect(mainPage, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(mainPage, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    connect(mainPage, SIGNAL(showUpgradePage(QString)), this, SLOT(showUpgradePage(QString)));
    connect(mainPage, SIGNAL(showApplyDelegatePage(QString)), this, SLOT(showApplyDelegatePage(QString)));
    connect(mainPage, SIGNAL(refreshAccountInfo()), this, SLOT(refresh()));
    connect(mainPage, SIGNAL(newAccount()), this, SLOT(refresh()));
    connect(mainPage, SIGNAL(showTransferPage(QString)), this, SLOT(showTransferPage(QString)));
}
void Frame::showDelegatePage() {
    showTransferPage();
}
void Frame::showTransferPage() {
    closeCurrentPage();
	getAccountInfo();
	const QString& accountName = DataMgr::getInstance()->getCurrentAccount();
	transferPage = new TransferPage(accountName, centralWidget);
    transferPage->setAttribute(Qt::WA_DeleteOnClose);
    connect(transferPage, SIGNAL(accountChanged(QString)), this, SLOT(showTransferPage(QString)));
    connect(transferPage, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(transferPage, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    connect(transferPage, SIGNAL(showAccountPage(QString)), this, SLOT(showAccountPage(QString)));
    transferPage->show();
    currentPageNum = 3;
    functionBar->choosePage(3);
}
void Frame::showContactPage() {
    closeCurrentPage();
    getAccountInfo();
    contactPage = new ContactPage(centralWidget);
    connect(contactPage, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(contactPage, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    connect(contactPage, SIGNAL(gotoTransferPage(QString)), this, SLOT(showTransferPageWithAddress(QString)));
    contactPage->setAttribute(Qt::WA_DeleteOnClose);
    contactPage->show();
    currentPageNum = 4;
}
void Frame::showUpgradePage(QString name) {
    closeCurrentPage();
    upgradePage = new UpgradePage(name, centralWidget);
    upgradePage->setAttribute(Qt::WA_DeleteOnClose);
    connect(upgradePage, SIGNAL(back(QString)), this, SLOT(showAccountPage(QString)));
    connect(upgradePage, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(upgradePage, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    connect(upgradePage, SIGNAL(showApplyDelegatePage(QString)), this, SLOT(showApplyDelegatePage(QString)));
    connect(upgradePage, SIGNAL(accountRenamed()), this, SLOT(refresh()));
    upgradePage->show();
    currentPageNum = 7;
}
void Frame::showApplyDelegatePage(QString name) {
    closeCurrentPage();
    applyDelegatePage = new ApplyDelegatePage(name, centralWidget);
    applyDelegatePage->setAttribute(Qt::WA_DeleteOnClose);
    connect(applyDelegatePage, SIGNAL(back(QString)), this, SLOT(showAccountPage(QString)));
    connect(applyDelegatePage, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(applyDelegatePage, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    connect(applyDelegatePage, SIGNAL(accountRenamed()), this, SLOT(refresh()));
    applyDelegatePage->show();
    currentPageNum = 8;
}
bool Frame::eventFilter(QObject* watched, QEvent* e) {
    if( (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::KeyPress)  ) {
        updateTimer();
        
    } else if ( isHidden() && e->type() == QEvent::ApplicationActivate) { //zxlrun
        qDebug() << "e->type() : " << e->type();
        showNormalAndActive();
    }

    //currentDialog 上的鼠标事件也会移动 frame 和 本身
    //if( Goopal::getInstance()->currentDialogVector.contains(  (QWidget*)watched) )
    
    if (false) {
        if( e->type() == QEvent::MouseMove) {
            mouseMoveEvent( (QMouseEvent*)e);
            
        } else if( e->type() == QEvent::MouseButtonPress) {
            mousePressEvent( (QMouseEvent*)e);
            
        } else if( e->type() == QEvent::MouseButtonRelease) {
            mouseReleaseEvent( (QMouseEvent*)e);
        }
        
        //   Goopal::getInstance()->resetPosOfCurrentDialog();  // currentDialog 一起移动
        return false;
    }
    
    return false;
}
void Frame::shadowWidgetShow() {
    qDebug() << "shadowWidgetShow";
    shadowWidget->raise();
    shadowWidget->show();
}
void Frame::shadowWidgetHide() {
    qDebug() << "shadowWidgetHide";
    shadowWidget->hide();
}

void Frame::blockchainStarted() {
	if (timerUpdate == nullptr)
	{
		timerUpdate = new QTimer(this);
		connect(timerUpdate, SIGNAL(timeout()), this, SLOT(autoUpdate()));
		timerUpdate->start(15 * 60 * 1000);
	}
}

void Frame::showTransferPageWithAddress(QString address) {
    closeCurrentPage();
    getAccountInfo();
	const QString& accountName = DataMgr::getInstance()->getCurrentAccount();
	transferPage = new TransferPage(accountName, centralWidget);
    transferPage->setAttribute(Qt::WA_DeleteOnClose);
    transferPage->setAddress(address);
    connect(transferPage, SIGNAL(accountChanged(QString)), this, SLOT(showTransferPage(QString)));
    connect(transferPage, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect(transferPage, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    transferPage->show();
    currentPageNum = 3;
    functionBar->choosePage(3);
}
void Frame::setLanguage(QString language) {
    DLOG_QT_WALLET_FUNCTION_BEGIN;

    if (language == "Simplified Chinese") {
        menuTranslator.load(DataMgr::getDataMgr()->getWorkPath() + "language/qt_zh_CN.qm");
        translatorForTextBrowser.load(DataMgr::getDataMgr()->getWorkPath() + "language/widgets.qm");
        translator.load(DataMgr::getDataMgr()->getWorkPath() + "language/gop_simplified_Chinese.qm");
        QApplication::installTranslator(&menuTranslator);
        QApplication::installTranslator(&translatorForTextBrowser);
    } else if( language == "English") {
        translator.load(DataMgr::getDataMgr()->getWorkPath() + "language/gop_English.qm");
        QApplication::removeTranslator(&menuTranslator);
        QApplication::removeTranslator(&translatorForTextBrowser);
    }

    QApplication::installTranslator(&translator);
    
    if( titleBar != NULL) {     // 已经登录
        functionBar->retranslator();
        titleBar->retranslator();
        bottomBar->retranslator();
        shadowWidget->retranslator();
        retranslateTrayIconUi();
        
        switch (currentPageNum) {
            case 0:
                showMainPage();
                break;
                
            case 1:
                //            mainPage->retranslator(language);
                showAccountPage( currentAccount);
                break;
                
            case 2:
                //            delegatePage->retranslator(language);
                showDelegatePage();
                break;
                
            case 3:
                //            transferPage->retranslator(language);
                showTransferPage(currentAccount);
                break;
                
            case 4:
                //            contactPage->retranslator(language);
                showContactPage();
                break;
                
            case 5:
                break;
                
            case 6:
                break;
                
            case 7:
                //            upgradePage->retranslator();
                showUpgradePage(currentAccount);
                break;
                
            case 8:
                //            applyDelegatePage->retranslator();
                showApplyDelegatePage(currentAccount);
                break;
                
            default:
                break;
        }
    }
    
    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::showWaittingForSyncWidget() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    
    waitingForSync = new WaitingForSync(this);
    waitingForSync->setAttribute(Qt::WA_DeleteOnClose);
    waitingForSync->move(0, 0);
    connect( waitingForSync, SIGNAL(sync()), this, SLOT(syncFinished()));
#ifdef WIN32
    connect( waitingForSync, SIGNAL(minimum()), this, SLOT(showMinimized()));
#else
    connect( waitingForSync, SIGNAL(minimum()), this, SLOT(hide()));
#endif // WIN32//zxlrun
    connect( waitingForSync, SIGNAL(tray()), this, SLOT(hide()));
    connect( waitingForSync, SIGNAL(closeGOP()), qApp, SLOT(quit()));
    connect( waitingForSync, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect( waitingForSync, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    waitingForSync->show();
    DLOG_QT_WALLET_FUNCTION_END;
}

void Frame::showSelectAchainPathWidget() {
    auto selectGopPathWidget = new SelectGopPathWidget(this);
    selectGopPathWidget->setAttribute(Qt::WA_DeleteOnClose);
    selectGopPathWidget->move(0, 0);
    connect( selectGopPathWidget, SIGNAL(enter()), this, SLOT(showWaittingForSyncWidget()));
#ifdef WIN32
    connect( selectGopPathWidget, SIGNAL(minimum()), this, SLOT(showMinimized()));
#else
    connect( selectGopPathWidget, SIGNAL(minimum()), this, SLOT(hide()));
#endif // WIN32//zxlrun
    connect( selectGopPathWidget, SIGNAL(closeGOP()), qApp, SLOT(quit()));
    connect( selectGopPathWidget, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
    connect( selectGopPathWidget, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    selectGopPathWidget->show();
}

void Frame::openWallet(QString result)
{
	if (waitingForSync != nullptr)
	{
		waitingForSync->deleteLater();
		waitingForSync = nullptr;
	}

	DataMgr::getDataMgr()->walletSetTransactionScanning(true);

    if( result == "\"result\":null") {
        auto normalLogin = new NormalLogin(this);
        normalLogin->setGeometry(0, 0, 960, 580);
        normalLogin->show();
        connect( normalLogin, SIGNAL(login()), this, SLOT(alreadyLogin()));
#ifdef WIN32
        connect( normalLogin, SIGNAL(minimum()), this, SLOT(showMinimized()));
#else
        connect( normalLogin, SIGNAL(minimum()), this, SLOT(hide()));
#endif // WIN32//zxlrun
        connect( normalLogin, SIGNAL(closeGOP()), qApp, SLOT(quit()));
        connect( normalLogin, SIGNAL(tray()), this, SLOT(hide()));
        connect( normalLogin, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
        connect( normalLogin, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
        
    } else {
        auto firstLogin = new FirstLogin(this);
        firstLogin->setGeometry(0, 0, 960, 580);
        firstLogin->show();
        connect( firstLogin, SIGNAL(login()), this, SLOT(alreadyLogin()));
#ifdef WIN32
        connect( firstLogin, SIGNAL(minimum()), this, SLOT(showMinimized()));
#else
		connect(firstLogin, SIGNAL(minimum()), this, SLOT(hide()));
#endif // WIN32//zxlrun
        connect( firstLogin, SIGNAL(closeGOP()), qApp, SLOT(quit()));
        connect( firstLogin, SIGNAL(tray()), this, SLOT(hide()));
        connect( firstLogin, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
        connect( firstLogin, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
    }
    
    return;
}
void Frame::walletListAccounts(QString result) {
    if( result.startsWith("\"error\":"))
        return;
        
    if( result == "\"result\":[]") {
        //  if( Goopal::getInstance()->currenChain() == 1 && testImportDelegatesStarted == false)
        {
            //       importTestDelegateAccounts();
        }
        return;
    }
    
    //update  DataMgr::getDataMgr()->getAccountInfo()->clear();
    QStringList accountInfoList = result.split("},{");
    QStringList accountValuesList;
    
    foreach (QString str, accountInfoList) {

        int pos = str.indexOf("\"name\":\"") + 8;
        if( pos == 7)  break;   // 如果还没有账号
        QString accountName = str.mid(pos, str.indexOf("\",", pos) - pos);

        CommonAccountInfo info;
        info.name = accountName;
        //todo
        info.balances  = DataMgr::getDataMgr()->getAccountInfo()->value(accountName).balances;
        
        if( str.contains("\"delegate_info\":")) {
            info.isDelegate = true;
            
        } else {
            info.isDelegate = false;
        }
        
        int pos2 = str.indexOf( "\"owner_address\":\"") + 17;
        QString addr = str.mid( pos2, str.indexOf( "\"", pos2) - pos2);

		if (addr.isEmpty())
		{
			qDebug() << "";
		}

        info.address = addr;
        int pos3 = str.indexOf( "\"registration_date\":") + 21;
        QString registerTime = str.mid( pos3, str.indexOf( "\"", pos3) - pos3);
        
        if( registerTime != "1970-01-01T00:00:00") {
            info.registerTime = registerTime;
            
        } else {
            info.registerTime = "NO";
        }
        
        int pos4 = str.indexOf( "\"owner_key\":\"") + 13;
        QString ownerKey = str.mid( pos4, str.indexOf( "\"", pos4) - pos4);
        info.ownerKey = ownerKey;

        QMap<QString, CommonAccountInfo>* p_account_info_map = DataMgr::getDataMgr()->getAccountInfo();
        
        foreach (CommonAccountInfo tmp, *p_account_info_map) {
            if (tmp.address == info.address && tmp.name != accountName) {
                DataMgr::getDataMgr()->getAccountInfo()->remove(tmp.name);
                DataMgr::getDataMgr()->getAccountInfo()->insert(accountName, info);
                break;
            }
        }
        
        DataMgr::getDataMgr()->getAccountInfo()->insert(accountName, info);
        accountValuesList.append(info.address);
    }
    
    if( DataMgr::getDataMgr()->getCurrentAccount().isEmpty()) {
        // 默认是 map中第一个账户
        DataMgr::getDataMgr()->setCurrentAccount(DataMgr::getDataMgr()->getAccountInfo()->keys().at(0));
        
    } else if( !DataMgr::getDataMgr()->getAccountInfo()->keys().contains(DataMgr::getDataMgr()->getCurrentAccount())) {
        // 如果当前账户被自动重命名localxxx 或被删除
        QString autoChangedNamePrefix = DataMgr::getDataMgr()->getCurrentAccount() + "local";
        QStringList accounts = DataMgr::getInstance()->getAccountInfo()->keys();
        bool hasAccountRenamed = false;
        
        foreach (QString account, accounts) {
            if( account.startsWith(autoChangedNamePrefix)) {
                // 如果有自动重命名添加"local"的
                QString oldName = DataMgr::getDataMgr()->getCurrentAccount();
                DataMgr::getDataMgr()->setCurrentAccount(account);
                hasAccountRenamed = true;
                break;
            }
        }
        
        if( !hasAccountRenamed) {
            // 如果当前账户被删除 当前账户切换为map中第一个账户
            DataMgr::getDataMgr()->setCurrentAccount(accounts.at(0));
        }
    }

	auto currencyInfo = DataMgr::getCurrentCurrency();
	qDebug() << "asset type:" << currencyInfo.name;
    
	if (currencyInfo.isAsset()) {
        QString str("");
        DataMgr::getDataMgr()->walletAccountBalance(str);
    } else {
        foreach (CommonAccountInfo account, *(DataMgr::getInstance()->getAccountInfo())) {
			DataMgr::getInstance()->tokenQueryBalance(currencyInfo.contractId, account.name);
        }
    }
    
    return;
}
void Frame::walletAccountBalance(QString result) {
	QJsonParseError json_error;
	QJsonDocument json_doucment = QJsonDocument::fromJson(("{" + result + "}").toLatin1(), &json_error);
	QJsonObject json_object = json_doucment.object();
	QVariantList balances_array = json_object.value(QString("result")).toArray().toVariantList();

	auto accountInfoMap = DataMgr::getDataMgr()->getAccountInfo();
	for (auto itr = accountInfoMap->begin(); itr != accountInfoMap->end(); itr++)
	{
		itr->balances.clear();
	}

    for (int i = 0; i < balances_array.size(); i++)
	{
        QVariantList asset_infos = balances_array[i].toList();
		QString account_name = asset_infos.at(0).toString();

		CommonAccountInfo info = accountInfoMap->value(account_name);

		QVariantList balance_infos = asset_infos.at(1).toList();
		QVariantList balance_info;

        for (int j = 0; j < balance_infos.size(); j++)
		{
            balance_info = balance_infos[j].toList();
			QString asset_id = balance_info.at(0).toString();
			QString asset_amount = balance_info.at(1).toString();
			double amount = asset_amount.toDouble() / DataMgr::assetPrecision();
			asset_amount = doubleToStr(amount);

			const CurrencyInfo& currencyInfo = DataMgr::getInstance()->getCurrencyByConId(asset_id);
			info.balances.insert(currencyInfo.name, asset_amount);
		}
		(*accountInfoMap)[account_name] = info;
	}

	emit updateTotalBalance();
}

void Frame::walletLock(QString result) {
    if( lockPage ) {
        qDebug() << "already exist a lockpage";
        return;
    }
    
    if( result == "\"result\":null") {
        shadowWidgetHide();
        lockPage = new LockPage(this);
        lockPage->setAttribute(Qt::WA_DeleteOnClose);
        lockPage->setGeometry(0, 0, 960, 580);
        connect( lockPage, SIGNAL(unlock()), this, SLOT(unlock()));
#ifdef WIN32
        connect( lockPage, SIGNAL(minimum()), this, SLOT(showMinimized()));
#else
        connect( lockPage, SIGNAL(minimum()), this, SLOT(hide()));
#endif // WIN32//zxlrun
        connect( lockPage, SIGNAL(closeGOP()), qApp, SLOT(quit()));
        connect( lockPage, SIGNAL(tray()), this, SLOT(hide()));
        connect( lockPage, SIGNAL(showShadowWidget()), this, SLOT(shadowWidgetShow()));
        connect( lockPage, SIGNAL(hideShadowWidget()), this, SLOT(shadowWidgetHide()));
        lockPage->show();
        //   Goopal::getInstance()->hideCurrentDialog();
    }
}

void Frame::closeEvent(QCloseEvent *e) {
    hide();
    e->ignore();
}
void Frame::iconIsActived(QSystemTrayIcon::ActivationReason reason) {
    switch(reason) {
        //点击托盘显示窗口
        case QSystemTrayIcon::Trigger: {
#ifdef WIN32
            showNormalAndActive();
#else
            static bool needReset = false;//zxlrun
            bool isHide = isHidden();
            if (isHide) {
                qDebug() << "isHide : " << isHide;
                trayIcon->setContextMenu(trayIconMenuEmpty);
                needReset = true;
            } else if (needReset) {
                trayIcon->setContextMenu(trayIconMenu);
                needReset = false;
            }
            showNormalAndActive();
#endif // WIN32//zxlrun
            break;
        }
        
        default:
            break;
    }
}

void Frame::createTrayIconActions() {
    minimizeAction = new QAction(tr("Minimize"), this);
#ifdef WIN32
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(showMinimized()));
#else
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));
#endif // WIN32//zxlrun
    restoreAction = new QAction(tr("Restore"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormalAndActive()));
    quitAction = new QAction(tr("Quit"), this);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
}

void Frame::retranslateTrayIconUi() {
    if (minimizeAction != nullptr)
        minimizeAction->setText(tr("Minimize"));
    if (restoreAction != nullptr)
        restoreAction->setText(tr("Restore"));
    if (quitAction != nullptr)
        quitAction->setText(tr("Quit"));
}

void Frame::createTrayIcon() {
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayIconMenu);
}
void Frame::showNormalAndActive() {
    showNormal();
    activateWindow();
}
void Frame::rebuild() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;

    if( centralWidget) {
        delete centralWidget;
        centralWidget = NULL;
    }
    
    if( bottomBar) {
        delete bottomBar;
        bottomBar = NULL;
    }
    
    if( functionBar) {
        delete functionBar;
        functionBar = NULL;
    }
    
    if( titleBar) {
        delete titleBar;
        titleBar = NULL;
    }
    
    if( timer) {
        delete timer;
        timer = NULL;
    }
    
    if (timerUpdate) {
        timerUpdate->stop();
        delete timerUpdate;
        timerUpdate = NULL;
    }
    
    if (timerForAutoRefresh) {
        delete timerForAutoRefresh;
        timerForAutoRefresh = NULL;
    }
    
    // Goopal::getInstance()->currentDialogVector.clear();
    qDebug() << "proc close " ;
    qDebug() << "proc finished " ;
    showWaittingForSyncWidget();
    DLOG_QT_WALLET_FUNCTION_END;
}
// 提前载入，防止切换到别的界面显示不出来
void Frame::init() {
    DLOG_QT_WALLET_FUNCTION_BEGIN;
    /*
    Goopal::getInstance()->postRPC( toJsonFormat( "id_wallet_get_all_approved_accounts", "wallet_get_all_approved_accounts", QStringList() << "1"));
    
    //    Goopal::getInstance()->postRPC( toJsonFormat( "id_network_get_blocked_ips", "network_get_blocked_ips", QStringList() << "" ));
    
    //    Goopal::getInstance()->postRPC( toJsonFormat( "id_blockchain_list_delegates", "blockchain_list_delegates", QStringList() << "1" << "9999"));
    
    Goopal::getInstance()->postRPC( toJsonFormat( "id_blockchain_list_pending_transactions", "blockchain_list_pending_transactions", QStringList() << "" ));
    
    Goopal::getInstance()->postRPC(  toJsonFormat( "id_wallet_set_transaction_scanning", "wallet_set_transaction_scanning", QStringList() << "true" ) );
    */
    DLOG_QT_WALLET_FUNCTION_END;
}
void Frame::scan() {
    //Goopal::getInstance()->postRPC( toJsonFormat( "id_scan", "scan", QStringList() << "0"));
    //DataMgr::getDataMgr()->
}
#define TRX_LOG   "TOKEN:SMC:TRANSLOG:201611111111:asd:afd:10123:END"
#define BALANCE_1 "TOKEN:SMC:BLC:asd:0:END"
#define BALANCE_2 "TOKEN:SMC:BLC:asd:0:END"
void Frame::tokenQueryBalance(QString result) {
    QString answer;
    answer = Misc::readAll(RpcMgr::getInstance()->currentProcess());
    QStringList balanceList = answer.split(":");
    
    if (balanceList.at(0) == "TOKEN" && balanceList.at(2) == "BLC") {
        QString account = DataMgr::getInstance()->getAddrAccont(balanceList.at(3));
        double dbalance = balanceList.at(4).toDouble();
        dbalance = dbalance / 100000;
        DataMgr::getInstance()->setAccountTokenBalance(account, balanceList.at(1), doubleToStr(dbalance));
    }
    
	QString asset = DataMgr::getCurrCurrencyName();
    qDebug() <<"ans:"<< answer;
    qDebug() <<"res:"<< result;
    //TODO
    //DataMgr::setAccountTokenBalance(DataMgr::getInstance()->getCurrentAccount(), "SMC", "1523");
    updataView();
}

void Frame::tokenTransferTo(QString result) {
}
