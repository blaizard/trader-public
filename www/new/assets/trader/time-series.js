function TimeSeries(config) {
	this.data = [];
	this.sum = [];
	this.config = {
		getTimestamp: function (entry) {
			return entry[0];
		},
		getData: function (entry) {
			return entry[1];
		},
		supportDataAveraging: true,
		/**
		 * Maximum number of samples to keep
		 */
		maxSamples: 1000
	};
	// Merge the configuration
	for (var i in this.config) {
		if (config && config[i]) {
			this.config[i] = config[i];
		}
	}
}

/**
 * \brief Add data to the list
 *
 * If ascending is set to true, the input data will be in ascending order,
 * otherwise descending.
 */
TimeSeries.prototype.add = function(data, ascending) {
	// Special case, if the data are empty, do nothing
	if (!data.length) {
		return false;
	}

	// Find the index where to merge the data
	var index = 0;
	var dataStart = 0;
	var dataEnd = data.length - 1;

	// If data are already registered
	if (this.data.length) {

		var timestampStart = this.getBeginTimestamp();
		var timestampEnd = this.getEndTimestamp();
		var timestampStartNewData = this.config.getTimestamp((ascending) ? data[0] : data[dataEnd]);
		var timestampEndNewData = this.config.getTimestamp((ascending) ? data[dataEnd] : data[0]);

		if (timestampStartNewData > timestampEnd) {
			index = this.data.length;
		}
		else if (timestampEndNewData < timestampStart) {
			index = 0;
		}
		else if (ascending && timestampEndNewData > timestampEnd) {
			index = this.data.length;
			for (dataStart = 1; dataStart <= dataEnd; dataStart++) {
				if (this.config.getTimestamp(data[dataStart]) > timestampEnd) {
					break;
				}
			}
		}
		else if (ascending && timestampStartNewData < timestampStart) {
			index = 0;
			for (dataEnd -= 1; dataEnd >= dataStart; dataEnd--) {
				if (this.config.getTimestamp(data[dataEnd]) < timestampStart) {
					break;
				}
			}
		}
		else if (!ascending && timestampEndNewData > timestampEnd) {
			index = this.data.length;
			for (dataEnd -= 1; dataEnd >= dataStart; dataEnd--) {
				if (this.config.getTimestamp(data[dataEnd]) > timestampEnd) {
					break;
				}
			}
		}
		else if (!ascending && timestampStartNewData < timestampStart) {
			index = 0;
			for (dataStart = 1; dataStart <= dataEnd; dataStart++) {
				if (this.config.getTimestamp(data[dataStart]) < timestampStart) {
					break;
				}
			}
		}
		else {
			return false;
		}
	}

	// Insert the data in batch from index
	var index2 = index;
	for (var i = dataStart; i <= dataEnd; ++i) {
		this.data.splice(index, 0, [this.config.getTimestamp(data[i]), this.config.getData(data[i])]);
		this.sum.splice(index, 0, 0);
		index += (ascending) ? 1 : 0;
	}
	index -= (ascending) ? 1 : 0;

	// Update the sum between index and index2
	if (this.config.supportDataAveraging)
	{
		var indexBegin = Math.min(index2, index);
		var indexEnd = Math.max(index2, index);
		var sum = (indexBegin > 0) ? this.sum[indexBegin - 1] : 0;
		for (var i = indexBegin; i <= indexEnd; ++i) {
			sum += this.data[i][1];
			this.sum[i] = sum;
		}
		for (var i = indexEnd + 1; i < this.sum.length; ++i) {
			sum += this.data[i][1];
			this.sum[i] = sum;
		}
	}
	else
	{
		this.sum = [];
	}

	// Keep only the last maxSamples samples
	this.data.slice(Math.max(this.data.length - this.config.maxSamples, 0));
	this.sum.slice(Math.max(this.sum.length - this.config.maxSamples, 0));

	return true;
}

TimeSeries.prototype.getEndTimestamp = function() {
	return (this.data.length) ? this.data[this.data.length - 1][0] : 0;
}

TimeSeries.prototype.getBeginTimestamp = function() {
	return (this.data.length) ? this.data[0][0] : 0;
}

/**
 * Return true if the structure is empty
 */
TimeSeries.prototype.empty = function() {
	return (this.data.length == 0);
}

/**
 * Return max nb samples and average the value if the actual number
 * of samples are higher.
 */
TimeSeries.prototype.eachAverage = function(callback, maxNb) {
	if (maxNb === undefined) {
		maxNb = 100;
	}

	var nbPointsPerData = Math.max(1, Math.ceil(this.data.length / maxNb));

	for (var i = 0; i < this.data.length; i += nbPointsPerData) {
		var iEnd = Math.min(i + nbPointsPerData - 1, this.data.length - 1);
		var timestamp = (this.data[i][0] + this.data[iEnd][0]) / 2;
		var sumData = this.sum[iEnd] - ((i > 0) ? this.sum[i-1] : 0);

		callback(timestamp, sumData / (iEnd - i + 1));
	}
}

/**
 * \brief Return samples
 *
 * \param callback The callback to receive each samples function(timestamp, data)
 * \param maxNb Max number of samples, if unset, all the samples are displayed
 * \param mostRecent If true (default), the most recent samples are returned, otehrwise, the oldest
 */
TimeSeries.prototype.each = function(callback, maxNb, mostRecent) {
	if (mostRecent === undefined) {
		mostRecent = true;
	}
	const nbSamples = (typeof maxNb == "number") ? Math.min(this.data.length, maxNb) : this.data.length;
	const start = (mostRecent) ? this.data.length - nbSamples : 0;

	for (var i = start; i < start + nbSamples; ++i) {
		callback(this.data[i][0], this.data[i][1]);
	}
}
