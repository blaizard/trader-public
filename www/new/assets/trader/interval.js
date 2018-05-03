function Interval(config) {
	this.interval = {};
	for (var name in config) {
		this.interval[name] = {
			period: config[name].period || 5000,
			callback: config[name].callback || function() {},
			instance: null
		};
	}

	// Run all intervals
	for (var name in config) {
		this.update(name, this.interval[name].period);
	}
};

/**
 * Check if an interval exists
 */
Interval.prototype.isValid = function(name) {
	return (typeof this.interval[name] !== "undefined");
};

/**
 * Run and/or update the period of a specific interval
 */
Interval.prototype.update = function(name, period) {
	if (!this.isValid(name)) {
		return console.log("The interval " + name + " is not valid");
	}

	if (this.interval[name].instance && period == this.interval[name].period) {
		return;
	}

	if (!this.interval[name].instance || period < this.interval[name].period) {
		this.interval[name].callback();
	}

	this.stop(name);
	this.interval[name].period = period;
	this.interval[name].instance = setInterval(this.interval[name].callback, period);
};

/**
 * Stop a specific interval. If already stopped do nothing.
 * The interval must exist.
 */
Interval.prototype.stop = function(name) {
	if (!this.isValid(name)) {
		return console.log("The interval " + name + " is not valid");
	}
	if (this.interval[name].instance) {
		clearInterval(this.interval[name].instance);
		this.interval[name].instance = null;
	}
};

/**
 * Stops all running intervals
 */
Interval.prototype.destroy = function() {
	for (var name in this.interval) {
		this.stop(name);
	}
};
