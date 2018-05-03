var Manager = function() {
	this.data = {
		exchangeList: [],
		strategyList: [],
		traceList: [],
		threadList: [],
		managerInfo: {}
	};
	this.interval = new Interval({
		exchangeList: {
			period: 60000,
			callback: () => { this.updateExchangeList(); }
		},
		strategyList: {
			period: 60000,
			callback: () => { this.updateStrategyist(); }
		},
		traceList: {
			period: 2000,
			callback: () => { this.updateTraces(); }
		},
		threadList: {
			period: 10000,
			callback: () => { this.updateThreads(); }
		},
		managerInfo: {
			period: 5000,
			callback: () => { this.updateManager(); }
		}
	});
};

Manager.prototype.updateTraces = function() {
	return irAjaxJson("api/v1/manager/traces").success((data) => {
		this.data.traceList = data.list;
	});
};

Manager.prototype.updateExchangeList = function() {
	irAjaxJson("api/v1/exchange/list").success((data) => {
		// Delete the exchange previously created if there are more than new ones
		for (var i = 0; i<this.data.exchangeList.length - data.list.length; ++i) {
			var exchange = this.data.exchangeList.pop();
			exchange.destroy();
		}

		var isNotConnected = false;
		for (var i in data.list) {
			var val = data.list[i];
			var connectedTimestamp = (new Date()).getTime() - val.timestamp + val.timestampConnected;

			// Update previous exchange if any
			if (this.data.exchangeList[i] && this.data.exchangeList[i].getName() == val.name) {
				this.data.exchangeList[i].update(val.name, val.status, i, val.timestamp - val.timestampServer, connectedTimestamp);
			}
			else {
				var exchange = new Exchange(val.name, val.status, i, val.timestamp - val.timestampServer, connectedTimestamp, () => {
					this.interval.update("exchangeList", 2000);
				});
				if (this.data.exchangeList[i]) {
					this.data.exchangeList[i].destroy();
					this.data.exchangeList[i] = exchange;
				}
				else {
					this.data.exchangeList.push(exchange);
				}
			}

			// Check if any of the exchange if in a state other than connected
			isNotConnected |= !this.data.exchangeList[this.data.exchangeList.length - 1].isConnected();
		}

		// Change the uupdate rate depending on the connection status
		this.interval.update("exchangeList", (isNotConnected) ? 2000 : 60000);
	}).error(() => {
		for (var i in this.data.exchangeList) {
			this.data.exchangeList[i].destroy();
		}
		this.data.exchangeList = [];
	});
};

Manager.prototype.updateStrategyist = function() {
	irAjaxJson("api/v1/strategy/list").success((data) => {
		// Delete the strategies previously created if there are more than new ones
		for (var i = 0; i<this.data.strategyList.length - data.list.length; ++i) {
			var strategy = this.data.strategyList.pop();
			strategy.destroy();
		}

		for (var i in data.list) {
			// Update previous exchange if any
			if (this.data.strategyList[i] && this.data.strategyList[i].getName() == data.list[i].name) {
				this.data.strategyList[i].update(data.list[i].name, i);
			}
			else {
				var strategy = new Strategy(data.list[i].name, i, () => {
					this.interval.update("strategyList", 2000);
				});
				if (this.data.strategyList[i]) {
					this.data.strategyList[i].destroy();
					this.data.strategyList[i] = strategy;
				}
				else {
					this.data.strategyList.push(strategy);
				}
			}
		}

		this.interval.update("strategyList", 60000);
	}).error(() => {
		for (var i in this.data.strategyList) {
			this.data.strategyList[i].destroy();
		}
		this.data.strategyList = [];
	});
};

Manager.prototype.updateThreads = function() {
	return irAjaxJson("api/v1/manager/threads").success((data) => {
		this.data.threadList = data.list;
	});
}

Manager.prototype.updateManager = function() {
	return irAjaxJson("api/v1/manager").success((data) => {
		this.data.managerInfo = data;
	});
}