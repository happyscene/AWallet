#include "sundry.h"
#include "commandjson.h"

Sundry::Sundry() {
}

Sundry::~Sundry() {
}

void Sundry::about() {
	clearPrefixAndParams();
    setCommand(QString("about"));
	sendCommand();
}

void Sundry::getInfo() {
	clearPrefixAndParams();
    setCommand(QString("get_info"));
	sendCommand();
}

QString Sundry::getLoginJson(QString& user, QString& password) {
	clearPrefixAndParams();
    setCommand(QString("login"));
	insertParam(param_type::string_param, user);
	insertParam(param_type::string_param, password);
	return	buildRpcJson();
}

void Sundry::validateAddress(QString& public_key) {
	clearPrefixAndParams();
    setCommand(QString("validate_address"));
	insertParam(param_type::string_param, public_key);
	sendCommand();
}

void Sundry::rpcSetUserName(QString& user) {
	clearPrefixAndParams();
    setCommand(QString("rpc_set_username"));
	insertParam(param_type::string_param, user);
	sendCommand();
}

void Sundry::rpcSetPassword(QString& password){
	clearPrefixAndParams();
    setCommand(QString("rpc_set_password"));
	insertParam(param_type::string_param, password);
	sendCommand();
}

void Sundry::rpcStartServer(int& port) {
	clearPrefixAndParams();
    setCommand(QString("rpc_start_server"));
	QString param_value = QString::number(port);
	insertParam(param_type::integer_param, param_value);
	sendCommand();
}

void Sundry::ntpUpdateTime() {
	clearPrefixAndParams();
    setCommand(QString("ntp_update_time"));
	sendCommand();
}

void Sundry::diskUsage() {
	clearPrefixAndParams();
    setCommand(QString("disk_usage"));
	sendCommand();
}

void Sundry::networkAddNode(QString& node, QString command /*= "add"*/) {
	clearPrefixAndParams();
    setCommand(QString("network_add_node"));
	insertParam(param_type::string_param, node);
	insertParam(param_type::string_param, command);
	sendCommand();
}

void Sundry::networkGetInfo() {
	clearPrefixAndParams();
    setCommand(QString("network_get_info"));
	sendCommand();
}

void Sundry::networkGetConnectionCount() {
	clearPrefixAndParams();
    setCommand(QString("network_get_connection_count"));
	sendCommand();
}

void Sundry::networkGetPeerInfo(bool hide_firewalled_nodes) {
	clearPrefixAndParams();
    setCommand(QString("network_get_peer_info"));
	QString param_value = hide_firewalled_nodes ? "1" : "0";
	insertParam(param_type::bool_param, param_value);
	sendCommand();
}

void Sundry::networkListPotentialPeers() {
	clearPrefixAndParams();
    setCommand(QString("network_list_potential_peers"));
	sendCommand();
}

void Sundry::networkGetUpnpInfo() {
	clearPrefixAndParams();
    setCommand(QString("network_get_upnp_info"));
	sendCommand();
}

void Sundry::executeScript(QString& path_file) {
	clearPrefixAndParams();
    setCommand(QString("execute_script"));
	insertParam(param_type::string_param, path_file);
	sendCommand();
}

void Sundry::delegateGetConfig() {
	clearPrefixAndParams();
    setCommand(QString("delegate_get_config"));
	sendCommand();
}

void Sundry::delegateSetNetworkMinConnectionCount(int min_count) {
	clearPrefixAndParams();
    setCommand(QString("delegate_set_network_min_connection_count"));
	QString param_value = QString::number(min_count);
	insertParam(param_type::integer_param, param_value);
	sendCommand();
}

void Sundry::delegateSetBlockMaxTransactionCount(int trx_max_count) {
	clearPrefixAndParams();
    setCommand(QString("delegate_set_block_max_transaction_count"));
	QString param_value = QString::number(trx_max_count);
	insertParam(param_type::integer_param, param_value);
	sendCommand();
}

void Sundry::delegateSetBlockMaxSize(int blcok_size) {
	clearPrefixAndParams();
    setCommand(QString("delegate_set_block_max_size"));
	QString param_value = QString::number(blcok_size);
	insertParam(param_type::integer_param, param_value);
	sendCommand();
}

void Sundry::delegateSetTransactionMaxSize(int trx_max_size) {
	clearPrefixAndParams();
    setCommand(QString("delegate_set_transaction_max_size"));
	QString param_value = QString::number(trx_max_size);
	insertParam(param_type::integer_param, param_value);
	sendCommand();
}

void Sundry::delegateSetTransactionMinFee(int trx_min_fee) {
	clearPrefixAndParams();
    setCommand(QString("delegate_set_transaction_min_fee"));
	QString param_value = QString::number(trx_min_fee);
	insertParam(param_type::integer_param, param_value);
	sendCommand();
}

void Sundry::executeConsolerCommand(QString& command, QStringList& args, bool rpc_enabled) {
	clearPrefixAndParams();
	QString cmd_prefix = rpc_enabled ? "console_" : ">>>";
	QString cmd = cmd_prefix + command;
	setCommand(cmd);
	QString param_value;
	foreach(param_value, args) {
		insertParam(param_type::string_param, param_value);
	}
	sendCommand();
}

void Sundry::balance() {
	clearPrefixAndParams();
    setCommand(QString("balance"));
	sendCommand();
}
void Sundry::lock() {
	clearPrefixAndParams();
    setCommand(QString("lock"));
	sendCommand();
}
