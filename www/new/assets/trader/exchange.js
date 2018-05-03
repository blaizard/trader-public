function Exchange(name, status, id, timestampDelta, timestampConnected, ajaxErrorCallback) {

	this.update(name, status, id, timestampDelta, timestampConnected);

	this.ajaxErrorCallback = ajaxErrorCallback;
	this.balance = new Balance();
	this.initialBalance = new Balance();
	this.configuration = {};
	this.rates = [];
	this.transactions = [];
	this.orders = [];
	this.orderRecords = new TimeSeries({
		getTimestamp: function (entry) {
			return entry.timestamp;
		},
		getData: function (entry) {
			return entry;
		},
		supportDataAveraging: false
	});

	this.interval = new Interval({
		updateBalance: {
			period: 5000,
			callback: () => {
				if (!this.isReadOnly()) {
					this.updateBalance();
					this.updateInitialBalance();
				}
			}
		},
		updateConfiguration: {
			period: 60000,
			callback: () => {this.updateConfiguration()}
		},
		updateTransactions: {
			period: 60000,
			callback: () => {this.updateTransactions()}
		},
		updateRates: {
			period: 60000,
			callback: () => {this.updateRates()}
		},
		updateOrders: {
			period: 60000,
			callback: () => {if (!this.isReadOnly()) this.updateOrders()}
		},
		updateOrderRecords: {
			period: 60000,
			callback: () => {if (!this.isReadOnly()) this.updateOrderRecords()}
		}
	});
};

Exchange.prototype.update = function(name, status, id, timestampDelta, timestampConnected) {
	this.name = name;
	this.status = status;
	this.id = id;
	this.timestampDelta = timestampDelta;
	this.timestampConnected = timestampConnected;
};

/**
 * Object destructor, must be called before remove
 */
Exchange.prototype.destroy = function() {
	this.interval.destroy();
};

Exchange.prototype.isConnected = function () {
	return (this.status == 3);
};

Exchange.prototype.isConnecting = function () {
	return (this.status == 2);
};

Exchange.prototype.isDisconnecting = function () {
	return (this.status == 1);
};

Exchange.prototype.isDisconnected = function () {
	return (this.status == 0);
};

Exchange.prototype.isConnectionError = function () {
	return (this.connectionError > 0);
};

Exchange.prototype.getStatus = function () {
	var statusText = ["disconnected", "disconnecting", "connecting", "connected"];
	return statusText[this.status];
};

Exchange.prototype.getTimestamp = function () {
	var d = new Date();
	return d.getTime() - this.timestampDelta;
};

Exchange.prototype.getTimestampConnected = function () {
	return this.timestampConnected;
};

Exchange.prototype.getName = function () {
	return this.name;
};

Exchange.prototype.getId = function () {
	return this.id;
};

Exchange.prototype.getBalance = function () {
	return this.balance;
};

Exchange.prototype.setConnectionError = function () {
	this.connectionError++;
};

Exchange.prototype.updateBalance = function () {
	return irAjaxJson("api/v1/exchange/" + this.id + "/balance").success((data) => {
		this.balance.update(data);
	}).error(() => {
		this.ajaxErrorCallback();
	});
};

Exchange.prototype.updateInitialBalance = function () {
	return irAjaxJson("api/v1/exchange/" + this.id + "/initial/balance").success((data) => {
		this.initialBalance.update(data);
	}).error(() => {
		this.ajaxErrorCallback();
	});
};

Exchange.prototype.updateConfiguration = function () {
	return irAjaxJson("api/v1/exchange/" + this.id + "/configuration").success((data) => {
		this.configuration = data;
	}).error(() => {
		this.ajaxErrorCallback();
	});
};

Exchange.prototype.updateTransactions = function () {
	return irAjaxJson("api/v1/exchange/" + this.id + "/transactions").success((data) => {
		this.transactions = data.list;
	}).error(() => {
		this.ajaxErrorCallback();
	});
};

Exchange.prototype.updateRates = function () {
	return irAjaxJson("api/v1/exchange/" + this.id + "/rates").success((data) => {
		this.rates = data.list;
	}).error(() => {
		this.ajaxErrorCallback();
	});
};

Exchange.prototype.updateOrders = function () {
	return irAjaxJson("api/v1/exchange/" + this.id + "/orders/active").success((data) => {
		this.orders = data.list;
	}).error(() => {
		this.ajaxErrorCallback();
	});
};

Exchange.prototype.updateOrderRecords = function () {
	return irAjaxJson("api/v1/exchange/" + this.id + "/orders").success((data) => {
		this.orderRecords.add(data.list);
	}).error(() => {
		this.ajaxErrorCallback();
	});
};

Exchange.prototype.hasBalance = function () {
	return this.balance.isValid();
};

Exchange.prototype.isReadOnly = function () {
	return (!this.configuration || this.configuration.readOnly);
};
