function Trader(view) {
	this.view = view;
	this.exchanges = [];
	this.strategies = [];
}

Trader.selectedView = {};
Trader.monitorList = [];

/**
 * Add a pair to be monitored
 */
Trader.addMonitor = function (exchangeId, pair) {
	if (exchangeId >= Trader.monitorList.length) {
		Trader.monitorList[exchangeId] = {};
	}
	Trader.monitorList[exchangeId][pair] = true;
};

Trader.getExchangeConfiguration = function (index, callback) {
	Trader.get("api/v1/exchange/" + index + "/configuration", function(data) {
		callback.call(this, data);
	}, function() {
		alert("error");
	});
};

Trader.changeView = function (type, args) {
	if (typeof args === "undefined") {
		args = {};
	}
	View.preloadView();
	Trader.selectedView = {type: type, new: args};
};

Trader.prototype.initialize = function () {
	Trader.changeView("dashboard");
	this.repeat(1, function(counterS) {
		return this.stateMachine(counterS);
	}, 0);
}

/**
 * Main function, running every seconds
 */
Trader.prototype.stateMachine = function (counterSeconds) {
	var commonTasks = function () {};

	// Means a new view is being created
	if (typeof Trader.selectedView.new !== "undefined") {
		// Merge the arguments
		{
			$.extend(Trader.selectedView, Trader.selectedView.new);
			delete Trader.selectedView.new;
		}
		// Set the exchange
		switch (Trader.selectedView.type) {
		case "dashboard":
			this.view.setDashboard();
			break;
		case "exchange":
			var index = Trader.selectedView.index;
			this.view.setExchange(this.exchanges[index]);
			break;
		case "strategy":
			var index = Trader.selectedView.index;
			this.view.setStrategy(this.strategies[index]);
			break;
		}
		counterSeconds = 0;
	}

	// Select the tasks
	switch (Trader.selectedView.type) {
	case "dashboard":
		commonTasks = function (counterSeconds) {
			// Every seconds
			this.updateManager();

			for (var i in this.exchanges) {
				if (this.exchanges[i].isConnected()) {
					// Update exchange rates of the exchanges that are connected (only)
					this.updatePairMonitor(i, "BTC/USD");
				}
			}

			// Every 10 seconds
			if ((counterSeconds % 10) == 0) {
				this.updateTraces();
				this.updateThreads();
			}
		};
		break;
	case "exchange":
		var index = Trader.selectedView.index;
		commonTasks = function (counterSeconds) {
			// First set the transaction list and then update it
			if (counterSeconds == 0) {
				this.updateTransactionsList(index);
			}
			else {
				this.updateTickerList(index);
			}

			// Only needs to be updated every 5 seconds
			if ((counterSeconds % 5) == 0) {
				this.updateActiveOrderList(index);
				this.updateOrderList(index);
				this.updateBalance(index);
				this.updateInitialBalance(index);
			}

			// Also monitor the pairMonitor
			for (var pair in Trader.monitorList[index]) {
				this.updatePairMonitor(index, pair);
			}
		};
		break;
	case "strategy":
		var index = Trader.selectedView.index;
		commonTasks = function (counterSeconds) {
			// Only needs to be updated every 5 seconds
			if ((counterSeconds % 5) == 0) {
				this.updateStrategyProfit(index);
			}
		};
		break;
	}

	// If no exchange or any of the exchange not connected,
	// run the updateExchangeList function
	var needToUpdateExchangeList = (this.exchanges.length == 0);
	for (var i in this.exchanges) {
		var isExchangeDown = (!this.exchanges[i].isConnected() || this.exchanges[i].isConnectionError());
		needToUpdateExchangeList |= isExchangeDown;
		// If the current exchange is down and is selected, switch back to the main view
		if (isExchangeDown && Trader.selectedView.type == "exchange" && Trader.selectedView.index == i) {
			Trader.changeView("dashboard");
		}
	}

	if (needToUpdateExchangeList) {
		counterSeconds = 0;
		this.updateExchangeList(function() {
			commonTasks.call(this, 0);
		});
		this.updateStrategyList();
	}
	else {
		commonTasks.call(this, counterSeconds);
	}

	return ++counterSeconds;
};

Trader.prototype.updateManager = function () {
	this.get("api/v1/manager", function (data) {
		this.view.updateManager(data);
	});
};

Trader.prototype.updateTraces = function () {
	this.get("api/v1/manager/traces", function (data) {
		var traces = {warnings: {timestamp: 0, list: []}, errors: {timestamp: 0, list: []}};
		for (var i in data.list) {
			var type = data.list[i].level;
			if (type == "w") {
				if (traces.warnings.timestamp == 0) {
					traces.warnings.timestamp = data.list[i].timestamp;
				}
				traces.warnings.list.push(data.list[i]);
			}
			else {
				if (traces.errors.timestamp == 0) {
					traces.errors.timestamp = data.list[i].timestamp;
				}
				traces.errors.list.push(data.list[i]);
			}
		}
		this.view.updateTraces(traces);
	});
};

Trader.prototype.updateThreads = function () {
	this.get("api/v1/manager/threads", function (data) {
		if (data.list) {
			console.log(data.list);
			data.list.sort(function(a, b){
				return (a.name < b.name) ? -1 : 1;
			});
			this.view.updateThreads(data.list);
		}
	});
};

Trader.prototype.updateActiveOrderList = function (exchangeId) {
	this.get("api/v1/exchange/" + exchangeId + "/orders/active", function (data) {
		if (data.list) {
			this.view.updateActiveOrderList(data.list);
		}
	});
};

Trader.prototype.updateOrderList = function (exchangeId) {
	this.get("api/v1/exchange/" + exchangeId + "/orders", function (data) {
		if (data.list) {
			this.view.updateOrderList(data.list);
		}
	});
};

Trader.prototype.updateBalance = function (exchangeId) {
	this.get("api/v1/exchange/" + exchangeId + "/balance", function (data) {
		this.view.updateBalance(data);
	});
};

Trader.prototype.updateInitialBalance = function (exchangeId) {
	this.get("api/v1/exchange/" + exchangeId + "/initial/balance", function (data) {
		this.view.updateInitialBalance(data);
	});
};

Trader.prototype.updateTransactionsList = function (exchangeId) {
	this.get("api/v1/exchange/" + exchangeId + "/transactions", function (data) {
		if (data.list) {
			this.view.updateTransactionsList(data.list);
		}
	});
};

Trader.prototype.updateTickerList = function (exchangeId) {
	this.get("api/v1/exchange/" + exchangeId + "/rates", function (data) {
		if (data.list) {
			this.view.updateTransactionsList(data.list);
		}
	});
};

Trader.prototype.updatePairMonitor = function (exchangeId, pair) {
	var label = this.exchanges[exchangeId].getName() + " " + pair;
	this.get("api/v1/exchange/" + exchangeId + "/rates/" + pair, function (data) {
		if (data.list) {
			var dataPoints = [];
			for (var i in data.list) {
				dataPoints.push([Trader.toLocalTimestamp(data.list[i].t), data.list[i].r]);
			}
			this.view.updatePairMonitor(label, dataPoints);
		}
	}, function() {
		this.exchanges[exchangeId].setConnectionError();
	});
};

Trader.get = function (endpoint, successCallback, errorCallback) {
	var obj = this;
	$.getJSON(endpoint, function(data) {
		successCallback.call(obj, data);
	}).fail(function() {
		if (typeof errorCallback === "function") {
			errorCallback.call(obj);
		}
	});
};

Trader.prototype.get = function (endpoint, successCallback, errorCallback) {
	var obj = this;
	$.getJSON(endpoint, function(data) {
		successCallback.call(obj, data);
	}).fail(function() {
		if (typeof errorCallback === "function") {
			errorCallback.call(obj);
		}
	});
};

Trader.prototype.repeat = function (timeS, callback, args) {
	var obj = this;
	var repeatFct = function(args) {
		args = callback.call(obj, args);
		setTimeout(function() {
			repeatFct(args);
		}, timeS * 1000);
	};
	repeatFct(args);
};

Trader.prototype.updateExchangeList = function (callback) {
	this.get("api/v1/exchange/list", function (data) {
		if (data.list) {
			var obj = this;
			this.exchanges = [];
			obj.view.exchangeClear();
			$.each(data.list, function(key, val) {

				var connectedTimestamp = (new Date()).getTime() - val.timestamp + val.timestampConnected;
				var ex = new Exchange(val.name, val.status, key,
						val.timestamp - val.timestampServer, connectedTimestamp);
				obj.exchanges.push(ex);
				obj.view.exchangeAdd(ex);
			});
		}
		if (typeof callback === "function") {
			callback.call(this);
		}
	}, function() {
		this.view.exchangeClear();
	});
};

Trader.prototype.updateStrategyList = function () {
	this.get("api/v1/strategy/list", function (data) {
		if (data.list) {
			var obj = this;
			this.strategies = [];
			obj.view.strategyClear();
			$.each(data.list, function(key, val) {
				var st = new Strategy(val.name, obj.strategies.length);
				obj.strategies.push(st);
				obj.view.strategyAdd(st);
			});
		}
	}, function() {
		this.view.strategyClear();
	});
};

Trader.prototype.updateStrategyProfit = function (index) {
	this.get("api/v1/strategy/" + index + "/profit", function (data) {
		if (data.list) {
			this.view.updateStrategyProfit(data.list);
		}
	});
};

Trader.toLocalTimestamp = function (timestamp) {
	if (typeof Trader.toLocalTimestamp.timeZoneDifference === "undefined") {
		var d = new Date();
		Trader.toLocalTimestamp.timeZoneDifference = d.getTimezoneOffset() * 60 * 1000;
	}
	return timestamp - Trader.toLocalTimestamp.timeZoneDifference;
};
