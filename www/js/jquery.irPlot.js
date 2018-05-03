/**
 * jQuery Module Template
 *
 * This template is used for jQuery modules.
 *
 */

(function($) {
	/**
	 * \brief .\n
	 * Auto-load the irPlot modules for the tags with a data-irPlot field.\n
	 * 
	 * \alias jQuery.irPlot
	 *
	 * \param {String|Array} [action] The action to be passed to the function. If the instance is not created,
	 * \a action can be an \see Array that will be considered as the \a options.
	 * Otherwise \a action must be a \see String with the following value:
	 * \li \b create - Creates the object and associate it to a selector. \code $("#test").irPlot("create"); \endcode
	 *
	 * \param {Array} [options] The options to be passed to the object during its creation.
	 * See \see $.fn.irPlot.defaults a the complete list.
	 *
	 * \return {jQuery}
	 */
	$.fn.irPlot = function(arg, data1, data2) {
		var retval;
		// Go through each objects
		$(this).each(function() {
			retval = $().irPlot.x.call(this, arg, data1, data2);
		});
		// Make it chainable, or return the value if any
		return (typeof retval === "undefined") ? $(this) : retval;
	};

	/**
	 * This function handles a single object.
	 * \private
	 */
	$.fn.irPlot.x = function(arg, data1, data2) {
		// Load the default options
		var options = $.fn.irPlot.defaults;

		// --- Deal with the actions / options ---
		// Set the default action
		var action = "create";
		// Deal with the action argument if it has been set
		if (typeof arg === "string") {
			action = arg;
		}
		// If the module is already created and the action is not create, load its options
		if (action != "create" && $(this).data("irPlot")) {
			options = $(this).data("irPlot");
		}
		// If the first argument is an object, this means options have
		// been passed to the function. Merge them recursively with the
		// default options.
		if (typeof arg === "object") {
			options = $.extend(true, {}, options, arg);
		}
		// Store the options to the module
		$(this).data("irPlot", options);

		// Handle the different actions
		switch (action) {
		// Create action
		case "create":
			$.fn.irPlot.create.call(this);
			break;
		// Update the content of the table with new values
		case "update":
			$.fn.irPlot.update.call(this, data1, data2);
			break;
		};
	};

	$.fn.irPlot.create = function() {

		var options = $(this).data("irPlot");
		var plotOptions = {
			lines: {
				show: true
			},
			grid: {
				hoverable: true //IMPORTANT! this is needed for tooltip to work
			},
			tooltip: true,
			tooltipOpts: {
				content: "%s: %y (%x)",
				xDateFormat: "%H:%M:%S"
			},
			legend: {
				position: "nw"
			}
		};

		// Set Y axis
		switch (options.axisY.type) {
		case "append":
			plotOptions.yaxis = {
				tickFormatter: function (value, axis) {
					return parseFloat(Math.round(value * 100) / 100).toFixed(2) + options.axisY.text;
				}
			};
			break;
		case "memory":
			plotOptions.yaxis = {
				tickFormatter: function (value, axis) {
					var numberToString = function (value) {
						return "" + parseFloat(Math.round(value * 100) / 100).toFixed(2);
					};

					if (value < 1024 / 2) {
						return numberToString(value);
					}
					else if (value < 1024 * 1024 / 2) {
						return numberToString(value / 1024) + " kB";
					}
					else if (value < 1024 * 1024 * 1024 / 2) {
						return numberToString(value / (1024 * 1024)) + " MB";
					}
					return numberToString(value / (1024 * 1024 * 1024)) + " GB";
				}
			};
			break;
		default:
		}

		// Set X axis
		switch (options.axisX.type) {
		case "time":
			plotOptions.xaxis = {
				mode: "time",
				minTickSize: [1, "second"],
				timeformat: "%H:%M:%S"
			};
			break;
		default:
		}

		if ($(this).width() < 100) {
			$(this).width(100);
		}
		if ($(this).height() < 100) {
			$(this).height(100);
		}

		var plotObj = $.plot(this, [], plotOptions);
		$(this).data("flot-data", plotObj);
	};

	$.fn.irPlot.update = function (label, dataPoints) {

		if (typeof dataPoints === "undefined") {
			alert("label and dataPoints must be set to irPlot.update");
			return;
		}

		var options = $(this).data("irPlot");
		var plotObj = $(this).data("flot-data");

		// Update the data
		var data = plotObj.getData();

		// Look for the data that contains the label, or create it
		var index = -1;
		{
			for (var i in data) {
				if (data[i].label == label) {
					index = i;
					break;
				}
			}
			// If not found create it
			if (index == -1) {
				var dataObj = {
					label: label,
					data: [],
					color: options.colorList[data.length % options.colorList.length]
				};
				if (typeof options.labelList[label] === "object") {
					dataObj = $.extend(true, dataObj, options.labelList[label]);
				}
				data.push(dataObj);
				index = data.length - 1;
			}
		}

		// Add new to current data
		if (dataPoints.length > 0)
		{
			var refData = data[index].data;
			// Data are always contiguous, then only append to the left or right of existing data
			if (refData.length > 0) {
				var newerTimestamp = refData[0][0];
				var olderTimestamp = refData[refData.length-1][0];

				var newerTimestampNew = dataPoints[0][0];
				var olderTimestampNew = dataPoints[dataPoints.length-1][0];

				// Copy on the left of the array
				if (newerTimestampNew > newerTimestamp) {
					var newData = [];
					for (var i in dataPoints) {
						if (dataPoints[i][0] <= newerTimestamp) {
							break;
						}
						newData.push(dataPoints[i]);
					}
					data[index].data = newData.concat(refData);
				}
				// Copy on the right of the array
				else if (olderTimestampNew < olderTimestamp) {
					var newData = [];
					for (var i=dataPoints.length-1; i>=0; --i) {
						if (dataPoints[i][0] >= olderTimestamp) {
							break;
						}
						newData.unshift(dataPoints[i]);
					}
					data[index].data = refData.concat(newData);
				}
			}
			// Copy the whole thing
			else {
				data[index].data = dataPoints;
			}
		}

		// Only Keep the last data
		data[index].data = data[index].data.slice(0, options.maxDataPoints);

		// Update the data
		plotObj.setData(data);
		plotObj.setupGrid();
		plotObj.draw();
	};

	/**
	 * \brief Default options, can be overwritten. These options are used to customize the object.
	 * Change default values:
	 * \code $().irPlot.defaults.theme = "aqua"; \endcode
	 * \type Array
	 */
	$.fn.irPlot.defaults = {
		/**
		 * Specifies a custom theme for this element.
		 * By default the irPlot class is assigned to this element, this theme
		 * is an additional class to be added to the element.
		 * \type String
		 */
		theme: "",
		/**
		 * Maximum number of data to keep
		 */
		maxDataPoints: 100,
		/**
		 * Axis X type
		 */
		axisX: {
			type: "time",
			legend: null
		},
		/**
		 * Axis Y type
		 */
		axisY: {
			type: "number",
			legend: null
		},

		/**
		 * List of color automatically assigned
		 */
		colorList: ["#f1c40f", "#27ae60", "#f39c12", "#8e44ad", "#2c3e50", "#c0392b"],
		/**
		 * Preset for labels
		 */
		labelList: {
			// color
		}
	};
})(jQuery);
