function Balance() {
	this.data = null;
	this.timeseries = new TimeSeries();
};

Balance.prototype.update = function(data) {
	this.data = data;
	this.timeseries.add([[(new Date()).getTime(), this.getEstimate()]], /*ascending*/true);
};

Balance.prototype.isValid = function() {
	return (this.data && this.data.total.estimate);
};

Balance.prototype.each = function(callback) {
	if (this.isValid()) {
		var i = 0;
		for (var currency in this.data) {
			if (["estimate", "total"].indexOf(currency) == -1) {
				callback(currency, this.data[currency], i++);
			}
		}
	}
};

Balance.prototype.getEstimateCurrency = function() {
	if (this.isValid()) {
		return this.data.estimate;
	}
	return "USD";
}

Balance.prototype.getEstimate = function() {
	if (this.isValid()) {
		return this.data.total.estimate;
	}
	return 0;
}

Balance.prototype.getInitialEstimate = function() {
	if (this.isValid()) {
		return this.data.total.initial;
	}
	return 0;
}

Balance.prototype.getEstimateString = function() {
	return this.getEstimate().toFixed(1) + " " + this.getEstimateCurrency();
}

Balance.prototype.getProfit = function() {
	if (this.isValid()) {
		return (this.getEstimate() / this.getInitialEstimate()) - 1;
	}
	return 0;
}
