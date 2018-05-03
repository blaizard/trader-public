function Strategy(name, id, ajaxErrorCallback) {
	this.update(name, id);

	this.ajaxErrorCallback = ajaxErrorCallback;
	this.profit = [];
	this.configuration = {};
	this.data = {};
	this.status = {};
	this.operationList = [];

	this.interval = new Interval({
		updateStatus: {
			period: 2000,
			callback: () => {this.updateStatus()}
		},
		updateProfit: {
			period: 5000,
			callback: () => {this.updateProfit(); }
		},
		updateConfiguration: {
			period: 60000,
			callback: () => {this.updateConfiguration()}
		},
		updateData: {
			period: 60000,
			callback: () => {this.updateData()}
		},
		updateOperationList: {
			period: 5000,
			callback: () => {this.updateOperationList()}
		}
	});
};

Strategy.prototype.update = function(name, id) {
	this.name = name;
	this.id = id;
};

/**
 * Object destructor, must be called before remove
 */
Strategy.prototype.destroy = function() {
	this.interval.destroy();
};

Strategy.prototype.getName = function () {
	return this.name;
};

Strategy.prototype.getId = function () {
	return this.id;
};

Strategy.prototype.getProfit = function () {
	return this.profit;
}

Strategy.prototype.getProcessTimePercent = function () {
	return (this.status) ? this.status.processTime : 0;
}

Strategy.prototype.updateStatus = function () {
	return irAjaxJson("api/v1/strategy/" + this.id + "/status").success((data) => {
		this.status = data;
	}).error(() => {
		this.ajaxErrorCallback();
	});
};

Strategy.prototype.updateConfiguration = function () {
	return irAjaxJson("api/v1/strategy/" + this.id + "/configuration").success((data) => {
		this.configuration = data;
	}).error(() => {
		this.ajaxErrorCallback();
	});
};

Strategy.prototype.updateData = function () {
	return irAjaxJson("api/v1/strategy/" + this.id + "/data").success((data) => {
		this.data = data;
	}).error(() => {
		this.ajaxErrorCallback();
	});
};

Strategy.prototype.updateProfit = function () {
	return irAjaxJson("api/v1/strategy/" + this.id + "/profit").success((data) => {
		for (var i in data.list) {
			if (!this.profit[i]) {
				this.profit[i] = {
					balance: new Balance()
				}
			}
			this.profit[i].balance.update(data.list[i].profit);
			for (var j in data.list[i]) {
				if (j != "profit") {
					this.profit[i][j] = data.list[i][j];
				}
			}
		}
	}).error(() => {
		this.ajaxErrorCallback();
	});
};

Strategy.prototype.updateOperationList = function () {
	return irAjaxJson("api/v1/strategy/" + this.id + "/operations").success((data) => {
		this.operationList = data.list;
	}).error(() => {
		this.ajaxErrorCallback();
	});
};
