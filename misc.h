#ifndef __MISC_H__
#define __MISC_H__

#include <QStringList>
#include <QProcess>
#include <QVector>

#define CONTRACT_FILE_NAME "\\contract_config.dat"
#define UNLOCK_CMD_POSTFIX  "_not_first_login";
#define CONFIG_INI_FILENAME "config.ini"
#define IDE_PATH "AchainDevelopmentTool"
#define ACHAIN "ACHAIN"

#define MODULE_ACHAIN "ACHAINDEVELOPMENTTOOL"

#ifdef WIN32
#define ACHAIN_WALLET_VERSION "2.0.3"
#else
#define ACHAIN_WALLET_VERSION "2.0.3"
#endif

#define ACHAIN_WALLET_VERSION_STR "v" ACHAIN_WALLET_VERSION

#define AUTO_REFRESH_TIME 10000
#define PWD_LOCK_TIME  7200
#define CHAIN_RPC_PORT 60010
#define ACHAIN_ACCOUNT_MAX 50
#define BTN_SELECTED_STYLESHEET   "QPushButton{ background:white;font: 14px \"微软雅黑\";border: 0px solid #000000;border-bottom: 3px solid rgb(68,217,199);}"
#define BTN_UNSELECTED_STYLESHEET "QPushButton{ background:white;font: 14px \"微软雅黑\";border: 0px solid #000000;border-bottom: 3px solid rgb(233,233,240);}"
#define HELP_HTML "https://www.achain.com/help/index.html"

//#define TEST

#ifdef TEST
    #define TOKEN_DOMAIN "http://172.16.33.201:8381/"
#else
    #define TOKEN_DOMAIN "https://api.achain.com/"
#endif

#define TOKEN_REQ TOKEN_DOMAIN "wallets/api/browser/act/contractTransactionAll?"
#define OTHER_TOKEN_REQ TOKEN_DOMAIN "wallets/api/browser/act/contractTransactions?"
#define CON_CONFOG_URL TOKEN_DOMAIN "wallets/api/browser/act/getAllContracts?status=2"

#define COMMONASSET "ACT"

struct CurrencyInfo {
	int id;
	QString contractId;
	QString name;
	QString coinType;

	bool isAsset()
	{
		return !contractId.startsWith("CON");
	}
	int assetId()
	{
		return contractId.toInt();
	}
};



struct ContractConfig {
    ContractConfig() : version(0) {};
    int version;
	QVector<CurrencyInfo> cons;
};


enum BlockChainType {
    Test = 0,
    Formal
};

struct SmartContractInfo {
    QString address;
    QString name;
    QString level;
    QString owner;
    QString ownerAddress;
    QString ownerName;
    QString state;
    QString description;
    QString balance;
    QStringList abiList;
    QStringList eventList;
};

template <class key, class value>
key getKeyByValue(QMap<key, value> m, value v);
QString toThousandFigure(int number); // 转换为001,015这种数字格式
QString doubleToStr(double number);
QString checkZero(double balance);

class Misc {
  public:
    static QString changePathFormat(QString path);
    static QString restorePathFormat(QString path);
    static bool isInContracts(QString filePath);
    static bool isInScripts(QString filePath);
    static qint64 write(QString cmd, QProcess* process);
    static QString read(QProcess* process);
    static QString readAll(QProcess *process);

    static bool isContractFileRegistered(QString path);
    static bool isContractFileUpgraded(QString path);
    static QString configGetContractAddress(QString path);
    static void configSetContractAddress(QString path, QString address);
};
#endif // Misc_H



