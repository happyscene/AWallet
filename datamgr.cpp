#include "datamgr.h"
#include <assert.h>
#include "rpcmgr.h"
#include "misc.h"

#include <qmessagebox.h>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>
#include <QVariantList>
#include <QDir>

DataMgr* DataMgr::_data_mgr = nullptr;
QMap<QString, QString> DataMgr::_trx_data;
CurrencyInfo DataMgr::_current_currency = { -1, "-1", "-1", "-1" };

DataMgr* DataMgr::getDataMgr() {
	if (!_data_mgr) {
		_data_mgr = new DataMgr;
	}
	return _data_mgr;
}

DataMgr* DataMgr::getInstance()
{
	return DataMgr::getDataMgr();
}

void DataMgr::deleteDataMgr() {
	if (_data_mgr) {
		delete _data_mgr;
	}
	_data_mgr = nullptr;
}

DataMgr::DataMgr() : timerGetAsset(nullptr) , _app_work_path("") {
	initSettings();
	fetchAssetInfo();
    setLanguage(QString("English"));
}

DataMgr::~DataMgr() {
    if (nullptr != timerGetAsset) {
        timerGetAsset->stop();
        delete timerGetAsset;
        timerGetAsset = nullptr;
    }

	unInitSettings();
}

bool currencyListCmpare(const CurrencyInfo& p1, const CurrencyInfo&  p2)
{
	return p1.name < p2.name;
}

void DataMgr::parseContract(QString& json_str) {
	QJsonParseError json_error;
	QJsonDocument json_doucment = QJsonDocument::fromJson(json_str.toUtf8(), &json_error);
	QJsonObject json_object = json_doucment.object();
	QJsonValue value = json_object.value(QString("result"));
	QJsonArray json_array;
	if (value.isArray()) {
		json_array = value.toArray();
	}

	_currency_list.clear();

	CurrencyInfo currencyInfo;
	QJsonObject con_object;
	int count = json_array.count();
	if (count > 0)
	{
		for (int i = 0; i < count; i++) {
			value = json_array.at(i);
			if (value.isObject()) {
				con_object = value.toObject();

				currencyInfo.id = con_object.value("id").toInt();
				currencyInfo.name = con_object.value("name").toString();

				if (currencyInfo.name == "ECT")
					continue;

				currencyInfo.contractId = con_object.value("contractId").toString();
				currencyInfo.coinType = con_object.value("coinType").toString();

				_currency_list.push_back(currencyInfo);
			}
		}
	}

	qSort(_currency_list.begin(), _currency_list.end(), currencyListCmpare);

	currencyInfo = { 0, "0", COMMONASSET, COMMONASSET };
	_currency_list.push_front(currencyInfo);
}

const QVector<CurrencyInfo>& DataMgr::getCurrencyList()
{
	return _currency_list;
}

void DataMgr::setCurrentCurrency(const CurrencyInfo& currencyInfo)
{
	_current_currency = currencyInfo;
}

CurrencyInfo DataMgr::getCurrentCurrency()
{
	return  _current_currency;
}

QString DataMgr::getCurrCurrencyName()
{
	return _current_currency.name;
}

CurrencyInfo DataMgr::getCurrencyById(int id)
{
	CurrencyInfo currencyInfo;

	for (auto info : _currency_list)
	{
		if (info.id == id)
		{
			currencyInfo = info;
			break;
		}
	}

	return currencyInfo;
}

CurrencyInfo DataMgr::getCurrencyByConId(const QString& contractId)
{
	CurrencyInfo currencyInfo;

	for (auto info : _currency_list)
	{
		if (info.contractId == contractId)
		{
			currencyInfo = info;
			break;
		}
	}

	return currencyInfo;
}

void DataMgr::fetchAssetInfo() {
	QSslConfiguration config;
	config.setPeerVerifyMode(QSslSocket::VerifyNone);
	config.setProtocol(QSsl::TlsV1_0);

	//next version contract from net
	QNetworkAccessManager* pManager = new QNetworkAccessManager(this);
	QNetworkRequest request;

	QString url(CON_CONFOG_URL);
	request.setUrl(QUrl(url));
	if (url.startsWith("https")) {
		request.setSslConfiguration(config);
	}
	QNetworkReply *pReply = pManager->get(request);
	connect(pReply, SIGNAL(finished()), this, SLOT(onFinished()));
	connect(pReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(onError(QNetworkReply::NetworkError)));
}

void DataMgr::onFinished() {
    QNetworkReply* network_reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray reply_content = network_reply->readAll();
    QString json_str(reply_content);

    if (QNetworkReply::NoError == network_reply->error() && 0 != json_str.size()) {
        parseContract(json_str);
        //saved result
        DataMgr::getInstance()->getSettings()->setValue("/settings/assetInfo", json_str);
        if (nullptr != timerGetAsset) {//timer
            timerGetAsset->stop();
            delete timerGetAsset;
            timerGetAsset = nullptr;
            emit assetTypeGet();
        }
    }
    else if (nullptr == timerGetAsset) {
        timerGetAsset = new QTimer(this);
        connect(timerGetAsset, SIGNAL(timeout()), this, SLOT(fetchAssetInfo()));
        timerGetAsset->start(10 * 60 * 1000);
    }
}

void DataMgr::onError(QNetworkReply::NetworkError errorCode){
    QNetworkReply *pReplay = qobject_cast<QNetworkReply*>(sender());
    qDebug() << errorCode;
    qDebug() << pReplay->errorString();
    //read saved result
    QString assetInfo = DataMgr::getDataMgr()->getSettings()->value("/settings/assetInfo").toString();
    parseContract(assetInfo);
    if (nullptr == timerGetAsset) {
        timerGetAsset = new QTimer(this);
        connect(timerGetAsset, SIGNAL(timeout()), this, SLOT(fetchAssetInfo()));
        timerGetAsset->start(10 * 1000);
    }
}

void DataMgr::setAccountTokenBalance(QString account, QString asset, QString balance)
{
	TokenAccountInfo tokenInfo;
	if (!DataMgr::getInstance()->getTokenAccountInfo()->contains(account)) {
		QMap<QString, TokenAccountInfo> assetInfo;
		DataMgr::getInstance()->getTokenAccountInfo()->insert(account, assetInfo);
	}

	QMap<QString, TokenAccountInfo>* assetInfo = &DataMgr::getInstance()->getTokenAccountInfo()->find(account).value();
	if (assetInfo->isEmpty()) {
		tokenInfo.address = DataMgr::getInstance()->getAccountInfo()->value(account).address;
		tokenInfo.name = DataMgr::getInstance()->getAccountInfo()->value(account).name;
		tokenInfo.balance = QString::number(0);
	} else {
		tokenInfo = assetInfo->value(asset);
		tokenInfo.balance = balance;
	}
	assetInfo->insert(asset, tokenInfo);
}

QString DataMgr::getAccountTokenBalance(QString account, QString asset)
{
	TokenAccountInfo tokenInfo;

	if (!_token_accounts_info_map.contains(account)) {
		QMap<QString, TokenAccountInfo> assetInfo;
		_token_accounts_info_map.insert(account, assetInfo);
	}

	QMap<QString, TokenAccountInfo>* assetInfo = &_token_accounts_info_map.find(account).value();
	if (assetInfo->isEmpty()) {
		tokenInfo.address = _account_info_map.value(account).address;
		tokenInfo.name = _account_info_map.value(account).name;
		tokenInfo.balance = QString::number(0);
		assetInfo->insert(asset, tokenInfo);
	}
	tokenInfo = assetInfo->value(asset);

	return tokenInfo.balance;
}

QString DataMgr::getAccountAddr(QString account)
{
	return _account_info_map.value(account).address;
}

QString DataMgr::getAddrAccont(QString addr)
{
	foreach(CommonAccountInfo acount, _account_info_map) {
        if (addr == acount.address) {
            return acount.name;
        }
    }
//zxlfor
//    for (QMap<QString, CommonAccountInfo>::iterator iter=_account_info_map.begin();
//         iter != _account_info_map.end();
//         ++iter)
//    {
//        if (addr == iter->address) {
//            return iter->name;
//        }
//    }
    return QString("");
}

void DataMgr::about()
{
	_sundry.about();
}

void DataMgr::getInfo() {
	_sundry.getInfo();
}

void DataMgr::validateAddress(QString& public_key) {
	_sundry.validateAddress(public_key);
}

void DataMgr::rpcSetUserName(QString& user) {
	_sundry.rpcSetUserName(user);
}

void DataMgr::rpcSetPassword(QString& password) {
	_sundry.rpcSetPassword(password);
}

void DataMgr::rpcStartServer(int& port) {
	_sundry.rpcStartServer(port);
}

void DataMgr::ntpUpdateTime() {
	_sundry.ntpUpdateTime();
}

void DataMgr::diskUsage() {
	_sundry.diskUsage();
}

void DataMgr::networkAddNode(QString& node, QString command/* = "add"*/) {
	_sundry.networkAddNode(node, command);
}

void DataMgr::networkGetInfo() {
	_sundry.networkGetInfo();
}

void DataMgr::networkGetConnectionCount() {
	_sundry.networkGetConnectionCount();
}

void DataMgr::networkGetPeerInfo(bool hide_firewalled_nodes) {
	_sundry.networkGetPeerInfo(hide_firewalled_nodes);
}

void DataMgr::networkListPotentialPeers() {
	_sundry.networkListPotentialPeers();
}

void DataMgr::networkGetUpnpInfo() {
	_sundry.networkGetUpnpInfo();
}

void DataMgr::executeScript(QString& path_file) {
	_sundry.executeScript(path_file);
}

void DataMgr::delegateGetConfig() {
	_sundry.delegateGetConfig();
}

void DataMgr::delegateSetNetworkMinConnectionCount(int min_count) {
	_sundry.delegateSetNetworkMinConnectionCount(min_count);
}

void DataMgr::delegateSetBlockMaxTransactionCount(int trx_max_count) {
	_sundry.delegateSetBlockMaxTransactionCount(trx_max_count);
}

void DataMgr::delegateSetBlockMaxSize(int blcok_size) {
	_sundry.delegateSetBlockMaxSize(blcok_size);
}
void DataMgr::delegateSetTransactionMaxSize(int trx_max_size) {
	_sundry.delegateSetTransactionMaxSize(trx_max_size);
}

void DataMgr::delegateSetTransactionMinFee(int trx_min_fee) {
	_sundry.delegateSetTransactionMinFee(trx_min_fee);
}

void DataMgr::executeConsolerCommand(QString& command, QStringList& args, bool rpc_enabled) {
	_sundry.executeConsolerCommand(command, args, rpc_enabled);
}

void DataMgr::balance() {
	_sundry.balance();
}

void DataMgr::lock() {
	_sundry.lock();
}

QString DataMgr::getLoginJson(QString& user, QString& password) {
	return _sundry.getLoginJson(user, password);
}

void DataMgr::blockChainGetBlockTransactions(QString& blcok_id) {
	_block_chain.getBlockTransactions(blcok_id);
}

void DataMgr::blockChainGetAccount(QString& account) {
	_block_chain.getAccount(account);
}

void DataMgr::blockChainGetInfo() {
	_block_chain.getInfo();
}

void DataMgr::blockChainGenerateSnapshot(QString& path_file) {
	_block_chain.generateSnapshot(path_file);
}

void DataMgr::blockChainIsSynced() {
	_block_chain.isSynced();
}

void DataMgr::blockChainGetBlockCount(){
	_block_chain.getBlockCount();
}

void DataMgr::blockChainGetBalance(QString& balance_id) {
	_block_chain.getBalance(balance_id);
}

void DataMgr::blockChainListKeyBalances(QString& public_key) {
	_block_chain.listKeyBalances(public_key);
}

void DataMgr::blockChainListActiveDelegates(int start_num, int limit_num) {
	_block_chain.listActiveDelegates(start_num, limit_num);
}

void DataMgr::blockChainListAccounts(int first, int limit) {
	_block_chain.listAccounts(first, limit);
}

void DataMgr::blockChainListPendingTransactions() {
	_block_chain.listPendingTransactions();
}

void DataMgr::blockChainGetTransaction(QString& trx_id, bool exact_search) {
	_block_chain.getTransaction(trx_id, exact_search);
}

void DataMgr::blockChainListDelegates(int first, int count) {
	_block_chain.listDelegates(first, count);
}

void DataMgr::blockChainListBlocks(int max_block_num, int limit) {
	_block_chain.listBlocks(max_block_num, limit);
}

void DataMgr::blockChainGetBlockSignee(QString& block) {
	_block_chain.getBlockSignee(block);
}

void DataMgr::blockChainGetBlock(int num) {
	_block_chain.getBlock(num);
}

void DataMgr::blockChainListAssets()
{
	_block_chain.getAssetsList();
}

void DataMgr::walletCreate(QString& name, QString& password) {
	_wallet.create(name, password);
}

void DataMgr::walletGetInfo() {
	_wallet.getInfo();
}

void DataMgr::walletClose() {
	_wallet.close();
}

void DataMgr::walletOpen(const QString& name) {
	_wallet.open(name);
}

void DataMgr::walletUnlock(int time_out, QString& password) {
	_wallet.unlock(time_out, password);
}

void DataMgr::walletImportPrivateKey(QString& wif_key_to_import, QString& account_name, bool create_account, bool rescan_blockchain) {
	_wallet.importPrivateKey(wif_key_to_import, account_name, create_account, rescan_blockchain);
}

void DataMgr::importRegisterPrivateKey(QString& wif_key_to_import, QString& account_name, bool create_account, bool rescan_blockchain) {
	_wallet.importRegisterPrivateKey(wif_key_to_import, account_name, create_account, rescan_blockchain);
}

void DataMgr::walletBackupCreate(QString& path_file) {
	_wallet.backupCreate(path_file);
}

void DataMgr::walletBackupRestore(QString& path_file, QString& name, QString& password) {
	_wallet.backupRestore(path_file, name, password);
}

void DataMgr::walletSetAutomaticBackups(bool enable) {
	_wallet.setAutomaticBackups(enable);
}

void DataMgr::walletSetTransactionExpirationTime(int time) {
	_wallet.setTransactionExpirationTime(time);
}

void DataMgr::walletAccountTransactionHistory(QString& account_name, QString& asset_name) {
	_wallet.accountTransactionHistory(account_name, asset_name);
}

void DataMgr::walletTransactionHistorySplite(QString& user, QString& asset_name, int limit, int trx_type) {
	_wallet.transactionHistorySplite(user, asset_name, limit, trx_type);
}

void DataMgr::walletTransactionHistorySpliteWithId(QString& user, QString& asset_name, int limit, int trx_type) {
	_wallet.transactionHistorySpliteWithId(user, asset_name, limit, trx_type);
}

void DataMgr::walletGetPendingTransactionErrors(QString& path_file) {
	_wallet.getPendingTransactionErrors(path_file);
}

void DataMgr::walletChangePassphrase(QString& old_password, QString& new_password) {
	_wallet.changePassphrase(old_password, new_password);
}

void DataMgr::walletCheckPassphrase(const QString& password) {
	_wallet.checkPassphrase(password);
}

void DataMgr::walletCheckAddress(QString& address) {
	_wallet.checkAddress(address);
}

void DataMgr::walletAccountCreate(QString& account_name) {
	_wallet.accountCreate(account_name);
}

void DataMgr::walletAccountSetApproval(QString& account_name, int approval) {
	_wallet.accountSetApproval(account_name, approval);
}

void DataMgr::walletTransferToAddress(QString& amount_to_transfer, 
	QString& asset_symbol, QString& from_account_name, 
	QString& to_address, QString& memo_message, QString& strategy) {
	_wallet.transferToAddress(amount_to_transfer, asset_symbol, from_account_name, to_address, memo_message, strategy);
}

void DataMgr::walletTransferToAddressWithId(QString unique_trx_id,
	QString& amount_to_transfer, QString& asset_symbol,
	QString& from_account_name, QString& to_address,
	QString& memo_message, QString& strategy) {
	_wallet.transferToAddressWithId(unique_trx_id, amount_to_transfer, asset_symbol, from_account_name, to_address, memo_message, strategy);
}

void DataMgr::walletTransferToPublicAccount(QString& amount_to_transfer, 
	QString& asset_symbol, QString& from_account_name, 
	QString& to_account_name, QString& memo_message, QString& strategy) {
	_wallet.transferToPublicAccount(amount_to_transfer, asset_symbol, from_account_name, to_account_name, memo_message, strategy);
}

void DataMgr::walletTransferToPublicAccountWithId(QString unique_trx_id,
	QString& amount_to_transfer, QString& asset_symbol,
	QString& from_account_name, QString& to_account_name,
	QString& memo_message, QString& strategy) {
	_wallet.transferToPublicAccountWithId(unique_trx_id, amount_to_transfer, asset_symbol, from_account_name, to_account_name, memo_message, strategy);
}

void DataMgr::walletRescanBlockchain() {
	_wallet.rescanBlockchain();
}

void DataMgr::walletCancelScan() {
	_wallet.cancelScan();
}

void DataMgr::walletGetTransaction(QString& transaction_id_prefix) {
	_wallet.getTransaction(transaction_id_prefix);
}

void DataMgr::walletAccountRegiste(QString& account_name, 
	QString& pay_from_account, QString& public_data, 
	int delegate_pay_rate, QString& account_type) {
	_wallet.accountRegiste(account_name, pay_from_account, public_data, delegate_pay_rate, account_type);
}

void DataMgr::walletListAccounts() {
	_wallet.listAccounts();
}

void DataMgr::walletListUnregisteredAccounts() {
	_wallet.listUnregisteredAccounts();
}

void DataMgr::walletListMyAddress() {
	_wallet.listMyAddress();
}

void DataMgr::walletListMyAccounts() {
	_wallet.listMyAccounts();
}

void DataMgr::walletGetAccountPublicAddress(QString& account) {
	_wallet.getAccountPublicAddress(account);
}

void DataMgr::walletAccountRename(QString& current_name, QString& new_name) {
	_wallet.accountRename(current_name, new_name);
}

void DataMgr::walletAccountBalance(QString& account_name) {
	_wallet.accountBalance(account_name);
}

void DataMgr::walletAccountBalanceIds(QString& account_name) {
	_wallet.accountBalanceIds(account_name);
}

void DataMgr::walletDelegateWithdrawPay(QString& delegate_name, QString& to_account_name, QString& amount_to_withdraw) {
	_wallet.delegateWithdrawPay(delegate_name, to_account_name, amount_to_withdraw);
}

void DataMgr::walletDelegatePayBalanceQuery(QString& account_name) {
	_wallet.delegatePayBalanceQuery(account_name);
}

void DataMgr::walletGetDelegateStatue(QString& account_name) {
	_wallet.getDelegateStatus(account_name);
}

void DataMgr::walletAccountDelete(QString& account_name) {
	_wallet.accountDelete(account_name);
}

void DataMgr::walletSetTransactionFee(QString& trx_fee) {
	_wallet.setTransactionFee(trx_fee);
}

void DataMgr::walletGetTransactionFee(QString& asset_name) {
	_wallet.getTransactionFee(asset_name);
}

void DataMgr::walletSetTransactionScanning(bool enable) {
	_wallet.setTransactionScanning(enable);
}

void DataMgr::walletDumpPrivateKey(QString& addr) {
	_wallet.dumpPrivateKey(addr);
}

void DataMgr::walletDelegateSetBlockProduction(QString& name, bool enable) {
	_wallet.delegateSetBlockProduction(name, enable);
}

void DataMgr::walletDumpAccountPrivateKey(QString& name, int type) {
	_wallet.dumpAccountPrivateKey(name, type);
}

void DataMgr::walletAccountUpdateRegistration(QString& account_name,
	QString& pay_from_account, QString& public_data, 
	int delegate_pay_rate, QString& account_type) {
	_wallet.accountUpdateRegistration(account_name, pay_from_account, public_data, delegate_pay_rate, account_type);
}


void DataMgr::tokenQueryTransferlog(QString conAddr, QString call_name)
{
    QString str("query_transfer_log");
    QString temp("");
    QString act("ACT");
    DataMgr::getInstance()->callContractTesting(conAddr, call_name, str, temp, act);
}

void DataMgr::tokenQueryBalance(QString contractId, QString call_name)
{
    QString str("query_balance");
    QString temp("");
    QString act("ACT");
	DataMgr::getInstance()->callContractTesting(contractId, call_name, str, temp, act);
}

void DataMgr::tokenTransferTo(QString call_name, QString contractId, QString to_address, double amount, double fee, QString remark)
{
	QString param = to_address + "|" + QString::number(amount, 'f', 5) + "|" + remark;
    QString str("transfer_to");
    QString temp("");
    QString act("ACT");
	DataMgr::getInstance()->callContract(contractId, call_name, str, param, act, fee);
}

void DataMgr::callContract(QString& contract, QString& call_name,
	QString& function_name, QString& params,
	QString& asset_symbol, double call_limit) {
	_contract.callContract(contract, call_name, function_name, params, asset_symbol, call_limit);
}

void DataMgr::callContractTesting(QString& contract, QString& call_name,
	QString& function_name, QString& params, QString& asset_symbol) {
	_contract.callContractTesting(contract, call_name, function_name, params, asset_symbol);
}

void DataMgr::walletTransferToContract(double amount_to_transfer, QString& asset_symbol,
	QString& from_account_name, QString& to_contract, double amount_for_exec) {
	_contract.walletTransferToContract(amount_to_transfer, asset_symbol, from_account_name, to_contract, amount_for_exec);
}

void DataMgr::walletTransferToContractTesting(double amount_to_transfer,
	QString& asset_symbol, QString& from_account_name, QString& to_contract) {
	_contract.walletTransferToContractTesting(amount_to_transfer, asset_symbol, from_account_name, to_contract);
}

void DataMgr::getContractInfo(QString& contract) {
	_contract.getContractInfo(contract);
}

void DataMgr::changeToToolConfigPath() {
	QFile file("config.ini");
	if (!file.exists())     return;
	QFile file2(_tool_config_path + "/config.ini");
	qDebug() << file2.exists() << _tool_config_path + "/config.ini";
	if (file2.exists())
	{
		qDebug() << "remove config.ini : " << file.remove();
		return;
	}

	qDebug() << "copy config.ini : " << file.copy(_tool_config_path + "/config.ini");
	qDebug() << "remove old config.ini : " << file.remove();
}

void DataMgr::initSettings() {
	getSystemEnvironmentPath();
	changeToToolConfigPath();

	updated = false;
	_config_file = new QSettings(_tool_config_path + "/config.ini", QSettings::IniFormat);
  _config_file->setIniCodec("utf8");
	_current_account = _config_file->value("/settings/testCurrentAccount").toString();

	QFile file(_tool_config_path + "/log.txt");
	file.open(QIODevice::Truncate | QIODevice::WriteOnly);
	file.close();
}

void DataMgr::unInitSettings() {
	if (_config_file) {
		delete _config_file;
		_config_file = nullptr;
	}
}

void DataMgr::getSystemEnvironmentPath() {
	QStringList environment = QProcess::systemEnvironment();
	QString str;

#ifdef WIN32
	foreach(str, environment) {
		if (str.startsWith("APPDATA=")) {
			_app_data_path = str.mid(8) + "\\ACHAINNew";
			_tool_config_path = str.mid(8) + "\\AchainDevelopmentToolNew";
			qDebug() << "appDataPath:" << _app_data_path;
			break;
		}
	}
#elif defined(TARGET_OS_MAC)
	foreach(str, environment) {
		if (str.startsWith("HOME=")) {
			_app_data_path = str.mid(5) + "/Library/Application Support/ACHAIN";
			_tool_config_path = str.mid(5) + "/Library/Application Support/AchainDevelopmentTool";
			qDebug() << "appDataPath:" << _app_data_path;
			break;
		}
    }
    _app_work_path = QCoreApplication::applicationDirPath() + "/";
    qDebug() << "_app_work_path : " << _app_work_path;
    //QMessageBox::information(NULL, "Title", _app_work_path, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
#else
	foreach(str, environment) {
		if (str.startsWith("HOME=")) {
			_app_data_path = str.mid(5) + "/ACHAIN";
			_tool_config_path = str.mid(5) + "/AchainDevelopmentTool";
			qDebug() << "appDataPath:" << _app_data_path;
			break;
		}
	}
#endif
}

void DataMgr::setCurrentAccount(QString accountName)
{
	RpcMgr* rpc_mgr = RpcMgr::getInstance();
	if (!rpc_mgr) {
		return;
	}

	_current_account = accountName;
	if (rpc_mgr->getCurChainType() == BlockChainType::Test) {
		_config_file->setValue("/settings/testCurrentAccount", accountName);
	}
	else {
		_config_file->setValue("/settings/formalCurrentAccount", accountName);
	}
}

QString DataMgr::getCurrentAccount()
{
	RpcMgr* rpc_mgr = RpcMgr::getInstance();
	if (!rpc_mgr) {
		return nullptr;
	}

	return  _current_account;
}

int DataMgr::assetPrecision()
{
	return 100000;
}

void DataMgr::parseTrxResultJson(QString trx_json, QVector<TrxResult>& trxes) {
	trxes.clear();

	QJsonParseError json_error;
	QJsonDocument json_doucment = QJsonDocument::fromJson(trx_json.toUtf8(), &json_error);
	QJsonObject json_object = json_doucment.object();
	QJsonValue value = json_object.value(QString("result"));
	if (!value.isArray()) {
		return;
	}

	Amount amountTmp;
	QMap<QString, Amount> amounts;

	QJsonArray json_array = value.toArray();
	int count = json_array.count();
	for (int i = 0; i < count; i++) {
		if (json_array.at(i).isObject()) {
			QJsonObject obj_value = json_array.at(i).toObject();
			TrxResult result;
			QJsonObject trx_object = obj_value;// value.toObject();
			result.vir = trx_object.value(QString("is_virtual")).toBool();
			result.confiremed = trx_object.value(QString("is_confirmed")).toBool();
			result.market = trx_object.value(QString("is_market")).toBool();
			result.market_cancel = trx_object.value(QString("is_market_cancel")).toBool();
			result.trx_id = trx_object.value(QString("trx_id")).toString();
			result.block_num = trx_object.value(QString("block_num")).toInt();
			result.block_position = trx_object.value(QString("block_position")).toInt();
			result.trx_type = trx_object.value(QString("trx_type")).toInt();
			result.time_stamp = trx_object.value(QString("timestamp")).toString();
			result.expiration_timestamp = trx_object.value(QString("expiration_timestamp")).toString();

			QJsonObject fee_object = trx_object.value(QString("fee")).toObject();
			result.fee.amount = (qint64)fee_object.value(QString("amount")).toDouble();
			result.fee.asset_id = fee_object.value(QString("asset_id")).toInt();
			
			QJsonArray entries_array = trx_object.value(QString("ledger_entries")).toArray();
			for (int j = 0; j < entries_array.count(); j++) {
				LedgerEntries entry;
				QJsonObject entry_object = entries_array.at(j).toObject();
				entry.from_account = entry_object.value(QString("from_account")).toString();
				entry.from_account_name = entry_object.value(QString("from_account_name")).toString();
				entry.to_account = entry_object.value(QString("to_account")).toString();
				entry.to_account_name = entry_object.value(QString("to_account_name")).toString();
				entry.memo = entry_object.value(QString("memo")).toString();
				if (entry.memo == QString(" "))
					entry.memo.clear();
				else if (entry.memo.startsWith("deposit toACT"))
					entry.memo.clear();

				QVariantList balances_array = entry_object.value(QString("running_balances")).toArray().toVariantList();

                for (int k = 0; k < balances_array.size(); k++)
				{
                    auto balances_obj = balances_array[k];

					QVariantList balances = balances_obj.toList();
					QString account = balances.at(0).toString();

					amounts.clear();

                    QVariantList amounts_array = balances.at(1).toList();
                    for (int l = 0; l < amounts_array.size(); l++)
					{
                        auto amounts_obj = amounts_array[l];

						QVariantList amounts_list = amounts_obj.toList();
						QMap<QString, QVariant> amount_obj = amounts_list.at(1).toMap();

						amountTmp.asset_id = amount_obj.value("asset_id").toInt();
						amountTmp.amount = amount_obj.value("amount").toDouble();

						const CurrencyInfo& currencyInfo = DataMgr::getInstance()->getCurrencyByConId(QString::number(amountTmp.asset_id));
						amounts.insert(currencyInfo.name, amountTmp);
					}

					if (amounts.size() > 0)
						entry.running_balances.insert(account, amounts);
				}
				
				QJsonObject amount_object = entry_object.value(QString("amount")).toObject();
                if (amount_object.value(QString("amount")).isString()){
					entry.amount.amount = (qint64)amount_object.value(QString("amount")).toString().toDouble();
				} else {
					entry.amount.amount = (qint64)amount_object.value(QString("amount")).toDouble();
				}
				entry.amount.asset_id = amount_object.value(QString("asset_id")).toInt();
				result.entries.push_back(entry);
			}

			trxes.push_back(result);
		}
	}
}

QMap<QString, QVector<TrxResult>>* DataMgr::getTrxResults() {
	return &_trx_result_map;
}

QMap<QString, CommonAccountInfo>* DataMgr::getAccountInfo() {
	return &_account_info_map;
}

void DataMgr::deleteAccountInfo(const QString& account)
{
	QMap<QString, CommonAccountInfo>::iterator itr = _account_info_map.find(account);
	if (itr != _account_info_map.end())
	{
		_account_info_map.remove(account);
	}
}

QMap<QString, QMap<QString, TokenAccountInfo>>* DataMgr::getTokenAccountInfo() {
	return &_token_accounts_info_map;
}

void DataMgr::onUpdateRpcResult(QString id, QString data) {
	qDebug() << "id" << id;
	if (id.startsWith("id_call_contract_testing")) {
		emit onCallContractTesting(data);
	} else if (id.startsWith("id_call_contract")) {
		emit onCallContract(data);
	} else if (id.startsWith("id_wallet_transfer_to_contract")) {
		emit onWalletTransferToContract(data);
	} else if (id.startsWith("id_wallet_transfer_to_contract_testing")) {
		emit onWalletTransferToContractTesting(data);
	} else if (id.startsWith("id_get_contract_info")) {
		emit onGetContractInfo(data);
	} else if (id.startsWith("id_about")) {
		emit onAbout(data);
	} else if (id.startsWith("id_get_info")) {
		emit onGetInfo(data);
	} else if (id.startsWith("id_validate_address")) {
		emit onValidateAddress(data);
	} else if (id.startsWith("id_rpc_set_username")) {
		emit onRpcSetUserName(data);
	} else if (id.startsWith("id_rpc_set_password")) {
		emit onRpcSetPassword(data);
	} else if (id.startsWith("id_rpc_start_server")) {
		emit onRpcStartServer(data);
	} else if (id.startsWith("id_ntp_update_time")) {
		emit onNtpUpdateTime(data);
	} else if (id.startsWith("id_disk_usage")) {
		emit onDiskUsage(data);
	} else if (id.startsWith("id_network_add_node")) {
		emit onNetworkAddNode(data);
	} else if (id.startsWith("id_network_get_info")) {
		emit onNetworkGetInfo(data);
	} else if (id.startsWith("id_network_get_connection_count")) {
		emit onNetworkGetConnectionCount(data);
	} else if (id.startsWith("id_network_get_peer_info")) {
		emit onNetworkGetPeerInfo(data);
	} else if (id.startsWith("id_network_list_potential_peers")) {
		emit onNetworkListPotentialPeers(data);
	} else if (id.startsWith("id_network_get_upnp_info")) {
		emit onNetworkGetUpnpInfo(data);
	} else if (id.startsWith("id_execute_script")) {
		emit onExecuteScript(data);
	} else if (id.startsWith("id_delegate_get_config")) {
		emit onDelegateGetConfig(data);
	} else if (id.startsWith("id_delegate_set_network_min_connection_count")) {
		emit onDelegateSetNetworkMinConnectionCount(data);
	} else if (id.startsWith("id_delegate_set_block_max_transaction_count")) {
		emit onDelegateSetBlockMaxTransactionCount(data);
	} else if (id.startsWith("id_delegate_set_block_max_size")) {
		emit onDelegateSetBlockMaxSize(data);
	} else if (id.startsWith("id_delegate_set_transaction_max_size")) {
		emit onDelegateSetTransactionMaxSize(data);
	} else if (id.startsWith("id_delegate_set_transaction_min_fee")) {
		emit onDelegateSetTransactionMinFee(data);
	} else if (id.startsWith("id_blockchain_get_block_transactions")) {
		emit onBlockChainGetBlockTransactions(data);
	} else if (id.startsWith("id_blockchain_get_account")) {
		emit onBlockChainGetAccount(data);
	} else if (id.startsWith("id_blockchain_get_info")) {
		emit onBlockChainGetInfo(data);
	} else if (id.startsWith("id_blockchain_generate_snapshot")) {
		emit onBlockChainGenerateSnapshot(data);
	} else if (id.startsWith("id_blockchain_is_synced")) {
		emit onBlockChainIsSynced(data);
	} else if (id.startsWith("id_blockchain_get_block_count")) {
		emit onBlockChainGetBlockCount(data);
	} else if (id.startsWith("id_blockchain_list_key_balances")) {
		emit onBlockChainListKeyBalances(data);
	} else if (id.startsWith("id_blockchain_list_active_delegates")) {
		emit onBlockChainListActiveDelegates(data);
	} else if (id.startsWith("id_blockchain_list_accounts")) {
		emit onBlockChainListAccounts(data);
	} else if (id.startsWith("id_blockchain_list_pending_transactions")) {
		emit onBlockChainListPendingTransactions(data);
	} else if (id.startsWith("id_blockchain_get_transaction")) {
		emit onBlockChainGetTransaction(data);
	} else if (id.startsWith("id_blockchain_list_delegates")) {
		emit onBlockChainListDelegates(data);
	} else if (id.startsWith("id_blockchain_list_blocks")) {
		emit onBlockChainListBlocks(data);
	} else if (id.startsWith("id_blockchain_get_block_signee")) {
		emit onBlockChainGetBlockSignee(data);
	} else if (id.startsWith("id_blockchain_get_block")) {
		emit onBlockChainGetBlock(data);
	} else if (id.startsWith("id_wallet_create")) {
		emit onWalletCreate(data);
	} else if (id.startsWith("id_wallet_get_info")) {
		emit onWalletGetInfo(data);
	} else if (id.startsWith("id_wallet_close")) {
		emit onWalletClose(data);
	} else if (id.startsWith("id_wallet_open")) {
		emit onWalletOpen(data);
	} else if (id.startsWith("id_wallet_unlock")) {
		emit onWalletUnlock(data);
	} else if (id.startsWith("id_wallet_import_private_key")) {
		emit onWalletImportPrivateKey(data);
	} else if (id.startsWith("register_id_wallet_import_private_key")){
		emit onWalletImportRegisterPrivateKey(data);
	} else if (id.startsWith("id_wallet_backup_create")) {
		emit onWalletBackupCreate(data);
	} else if (id.startsWith("id_wallet_backup_restore")) {
		emit onWalletBackupRestore(data);
	} else if (id.startsWith("id_wallet_set_automatic_backups")) {
		emit onWalletSetAutomaticBackups(data);
	} else if (id.startsWith("id_wallet_set_transaction_expiration_time")) {
		emit onWalletSetTransactionExpirationTime(data);
	} else if (id.startsWith("id_wallet_account_transaction_historys")) {
		emit onWalletAccountTransactionHistory(data);
	} else if (id.startsWith("id_wallet_transaction_history_splite_")) {
		emit onWalletTransactionHistorySpliteWithId(id, data);
	} else if (id.startsWith("id_wallet_transaction_history_splite")) {
		emit onWalletTransactionHistorySplite(data);
	} else if (id.startsWith("id_wallet_get_pending_transaction_errors")) {
		emit onWalletGetPendingTransactionErrors(data);
	} else if (id.startsWith("id_wallet_change_passphrase")) {
		emit onWalletChangePassphrase(data);
	} else if (id.startsWith("id_wallet_check_passphrase")) {
		emit onWalletCheckPassphrase(data);
	} else if (id.startsWith("id_wallet_check_address")) {
		emit onWalletCheckAddress(data);
	} else if (id.startsWith("id_wallet_account_create")) {
		emit onWalletAccountCreate(data);
	} else if (id.startsWith("id_wallet_account_set_approval")) {
		emit onWalletAccountSetApproval(data);
	} else if (id.startsWith("id_wallet_transfer_to_address_")) {
		emit onWalletTransferToAddressWithId(id, data);
	} else if (id.startsWith("id_wallet_transfer_to_address")) {
		emit onWalletTransferToAddress(data);
	} else if (id.startsWith("id_wallet_transfer_to_public_account_")) {
		emit onWalletTransferToPublicAccountWithId(id, data);
	} else if (id.startsWith("id_wallet_transfer_to_public_account")) {
		emit onWalletTransferToPublicAccount(data);
	} else if (id.startsWith("id_wallet_rescan_blockchain")) {
		emit onWalletRescanBlockchain(data);
	} else if (id.startsWith("id_wallet_cancel_scan")) {
		emit onWalletCancelScan(data);
	} else if (id.startsWith("id_wallet_get_transaction")) {
		emit onWalletGetTransaction(data);
	} else if (id.startsWith("id_wallet_account_register")) {
		emit onWalletAccountRegiste(data);
	} else if (id.startsWith("id_wallet_list_accounts")) {
		emit onWalletListAccounts(data);
	} else if (id.startsWith("id_wallet_list_unregistered_accounts")) {
		emit onWalletListUnregisteredAccounts(data);
	} else if (id.startsWith("id_wallet_list_my_addresses")) {
		emit onWalletListMyAddresses(data);
	} else if (id.startsWith("id_wallet_list_my_accounts")) {
		emit onWalletListMyAccounts(data);
	} else if (id.startsWith("id_wallet_get_account_public_address")) {
		emit onWalletGetAccountPublicAddress(data);
	} else if (id.startsWith("id_wallet_account_rename")) {
		emit onWalletAccountRename(data);
	} else if (id.startsWith("id_wallet_account_balance_ids")) {
		emit onWalletAccountBalanceIds(data);
	} else if (id.startsWith("id_wallet_account_balance")) {
		emit onWalletAccountBalance(data);
	} else if (id.startsWith("id_wallet_delegate_withdraw_pay")) {
		emit onWalletDelegateWithdrawPay(data);
	} else if (id.startsWith("id_wallet_delegate_pay_balance_query")) {
		emit onWalletDelegatePayBalanceQuery(data);
	} else if (id.startsWith("id_wallet_get_delegate_statue")) {
		emit onWalletGetDelegateStatue(data);
	} else if (id.startsWith("id_wallet_account_delete")) {
		emit onWalletAccountDelete(data);
	} else if (id.startsWith("id_wallet_set_transaction_fee")) {
		emit onWalletSetTransactionFee(data);
	} else if (id.startsWith("id_wallet_get_transaction_fee")) {
		emit onWalletGetTransactionFee(data);
	} else if (id.startsWith("id_wallet_set_transaction_scanning")) {
		emit onWalletSetTransactionScanning(data);
	} else if (id.startsWith("id_wallet_dump_private_key")) {
		emit onWalletDumpPrivateKey(data);
	} else if (id.startsWith("id_wallet_delegate_set_block_production")) {
		emit onWalletDelegateSetBlockProduction(data);
	} else if (id.startsWith("id_wallet_dump_account_private_key")) {
		emit onWalletDumpAccountPrivateKey(data);
	} else if (id.startsWith("id_wallet_account_update_registration")) {
		emit onWalletAccountUpdateRegistration(data);
	} else if (id.startsWith("id_console_") || id.startsWith(">>>")) {
		emit onExecuteConsoleCommand(data);
	} else if (id.startsWith("id_balance")) {
		emit onBalance(data);
	} else if (id.startsWith("id_lock")) {
		emit onLock(data);
	}
}
