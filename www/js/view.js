function View() {
}

View.loading = function () {
	return "<center><i class=\"fa fa-circle-o-notch fa-spin fa-3x fa-fw\"></i></center>";
};

View.showExchangeConfig = function (selector, index) {
	$(selector).html(View.loading());

	Trader.getExchangeConfiguration(index, function(data) {
		var content = $("<div/>", {
			class: "panel-body"
		});
		var table = $("<table/>", {
			class: "table table-bordered table-hover table-striped"
		});
		table.html("<thead><tr>"
            + "<th>Name</th>"
			+ "<th>Value</th>"
        	+ "</tr></thead>");
		var tbody = $("<tbody/>");

		$.each(data, function(key, val) {
			$(tbody).append("<tr><td>" + key + "</td><td>" + val + "</td></tr>");
		});

		$(table).append(tbody);
		$(content).append(table);

		$(selector).html(content);
	});
};

View.preloadView = function () {
	$("#page-wrapper").html(View.loading());
};

/**
 * This will set the dashboard view
 */
View.prototype.setDashboard = function () {
	$("#page-wrapper").html(
		"<div class=\"row\"><div class=\"col-lg-12\"><h1 class=\"page-header\">Dashboard</h1></div></div>"
		+ "<div class=\"row\" id=\"exchange-status\"></div>"
		+ "<div class=\"row\">"
			+ "<div class=\"col-lg-8\">"
				+ "<div class=\"panel panel-default\">"
					+ "<div class=\"panel-heading\"><i class=\"fa fa-bar-chart-o fa-fw\"></i> Estimated balance</div>"
					+ "<div class=\"panel-body\"><div id=\"pair-monitor\" style=\"width: 100%; min-height: 400px;\"></div></div>"
				+ "</div>"
				+ "<div class=\"panel panel-default\">"
					+ "<div class=\"panel-heading\"><i class=\"fa fa-list-ul\"></i> Lastest traces</div>"
					+ "<div class=\"panel-body\"><div id=\"manager-traces\" class=\"table-responsive\"></div></div>"
				+ "</div>"
			+ "</div>"
			+ "<div class=\"col-lg-4\">"
				+ "<div class=\"panel panel-default\">"
					+ "<div class=\"panel-heading\"><i class=\"fa fa-heart-o\"></i> Status</div>"
					+ "<div class=\"panel-body\">"
						+ "<div id=\"manager-memory\" style=\"width: 100%; min-height: 200px;\"></div>"
						+ "<div id=\"manager-table\" class=\"table-responsive\"></div>"
					+ "</div>"
				+ "</div>"
				+ "<div class=\"panel panel-default\">"
					+ "<div class=\"panel-heading\"><i class=\"fa fa-bell fa-fw\"></i> Notifications</div>"
					+ "<div class=\"panel-body\">"
                    	+ "<div id=\"manager-notifications\" class=\"list-group\">"
                    	+ "</div>"
					+ "</div>"
				+ "</div>"
				+ "<div class=\"panel panel-default\">"
					+ "<div class=\"panel-heading\"><i class=\"fa fa-tasks\"></i> Threads</div>"
					+ "<div class=\"panel-body\"><div id=\"manager-threads\" class=\"table-responsive\"></div></div>"
				+ "</div>"
			+ "</div>"
		+ "</div>"
	);


	$("#pair-monitor").irPlot();

	$("#manager-memory").irPlot({
		axisY : {
			type: "memory"
		},
		labelList: {
			"alloc": {
				lines: {
					fill: true,
					fillColor: {
						colors: [{ opacity: 0.3 }, { opacity: 1 }]
					}
				}
			}
		}
	});

	$("#manager-table").irTable({
		header: ["Type", "Value"],
		makeRow: function(item) {
			var content = "<tr>";
	    	content += "<td>" + item[0] + "</td>";
	    	content += "<td>" + item[1] + "</td>";
	    	content += "</tr>";
	    	return content;
		},
		isMatch: function(item1, item2) {
			return (item1[0] == item2[0]);
		},
		updateRow: function(row, item) {
			$(row).children("td:nth(1)").text(item[1]);
		}
	});

	$("#manager-traces").irTable({
		header: ["Time", "Type", "Topic", "Message"],
		makeRow: function(item) {
			var content = "<tr>";
	    	content += "<td>" + View.toDate(item.timestamp) + "</td>";
	    	content += "<td>" + item.level + "</td>";
	    	content += "<td>" + item.topic + "</td>";
	    	content += "<td>" + item.message + "</td>";
	    	content += "</tr>";
	    	return content;
		}
		, keepAll: true
	});

	$("#manager-threads").irTable({
		header: ["Status"],
		makeRow: function(item) {
			var content = "<tr>";
	    	content += "<td>"
	    	content += (item.isActive) ? "<i class=\"fa fa-circle\" style=\"color:#5cb85c;\"></i>"
	    			: "<i class=\"fa fa-circle\" style=\"color:#d43f3a;\"></i>";
	    	content += "&nbsp;" + item.name + "</td>";
	    	content += "</tr>";
	    	return content;
		}
	});
};

View.prototype.setExchange = function (exchange) {
	$("#page-wrapper").html(

		"<br/><div class=\"row\">"
			+ "<div class=\"col-lg-12 col-md-6\">"
	            + "<div class=\"panel panel-primary\">"
	                + "<div class=\"panel-heading\">"
	                	+ "<div class=\"row\">"
	                		+ "<div class=\"col-lg-12\">"
	                			+ "<h1>" + exchange.getName() + "</h1>"
	                		+ "</div>"
	                	+ "</div>"
	                	+ "<div class=\"row\">"
	                		+ "<div class=\"col-lg-3\">"
	                			+ "<div id=\"balance-estimate\"></div>"
	                		+ "</div>"
	                		+ "<div class=\"col-lg-3\">"
	                			+ "<div id=\"balance-initial-estimate\"></div>"
	                		+ "</div>"
	                		+ "<div class=\"col-lg-3\">"
	                			+ "<div id=\"active-orders-metrics\"></div>"
	                		+ "</div>"
	                		+ "<div class=\"col-lg-3\">"
	                			+ "<div id=\"active-pairs-metrics\"></div>"
	                		+ "</div>"
	                	+ "</div>"
	                + "</div>"
	            + "</div>"
	         + "</div>"
		+ "</div>"

		+ "<div class=\"row\">"
			+ "<div class=\"col-lg-6\">"
				+ "<div class=\"panel panel-default\">"
					+ "<div class=\"panel-heading\">"
		            	+ "<i class=\"fa fa-usd\"></i> Balance"
		            + "</div>"
		            + "<div class=\"panel-body\">"
		            	+ "<div id=\"balance-graph\" style=\"width: 100%; min-height: 200px;\"></div>"
		            	+ "<div id=\"balance-list\" class=\"table-responsive\">"
		            	+ "</div>"
		            + "</div>"
				+ "</div>"
			+ "</div>"
			+ "<div class=\"col-lg-6\">"
				+ "<div class=\"panel panel-default\">"
					+ "<div class=\"panel-heading\">"
		            	+ "<i class=\"fa fa-area-chart\"></i> Monitor"
		            + "</div>"
		            + "<div class=\"panel-body\">"
		            	+ "<div id=\"pair-monitor\" style=\"width: 100%; min-height: 400px;\"></div>"
		            + "</div>"
				+ "</div>"
			+ "</div>"
		+ "</div>"
		+ "<div class=\"panel panel-default\">"
			+ "<div class=\"panel-heading\">"
            	+ "<i class=\"fa fa-bar-chart-o fa-fw\"></i> Active Orders"
            + "</div>"
            + "<div class=\"panel-body\">"
                + "<div id=\"active-order-list\" class=\"table-responsive\"></div>"
            + "</div>"
		+ "</div>"
		+ "<div class=\"panel panel-default\">"
			+ "<div class=\"panel-heading\">"
            	+ "<i class=\"fa fa-bar-chart-o fa-fw\"></i> Orders Log"
            + "</div>"
            + "<div class=\"panel-body\">"
                + "<div id=\"order-list\" class=\"table-responsive\"></div>"
            + "</div>"
		+ "</div>"
		+ "<div class=\"panel panel-default\">"
			+ "<div class=\"panel-heading\">"
            	+ "<i class=\"fa fa-bar-chart-o fa-fw\"></i> Pairs"
            + "</div>"
            + "<div class=\"panel-body\">"
                + "<div id=\"pair-list\" class=\"table-responsive\"></div>"
            + "</div>"
		+ "</div>");

	$("#balance-graph").irPlot({
		axisY : {
			type: "append",
			text: " USD"
		}
	});

	$("#balance-list").irTable({
		header: ["Currency", "Funds", "Reserved", "~USD"],
		makeRow: function(item) {
			var content = "<tr class=\"currency-" + item.key + "\">";
	    	content += "<td data-mouseover=\"tr.currency-" + item.key + "\">" + item.key + "</td>";
	    	content += "<td>" + item.amount + "</td>";
	    	content += "<td>" + item.reserve + "</td>";
	    	content += "<td>" + item.estimate + "</td>";
	    	content += "</tr>";
	    	return content;
		},
		isMatch: function(item1, item2) {
			return (item1.key == item2.key);
		},
		updateRow: function(row, item) {
			$(row).children("td:nth(1)").text((item.amount < 0.0000001) ? 0 : item.amount);
			$(row).children("td:nth(2)").text(item.reserve);
			$(row).children("td:nth(3)").text((item.estimate < 0.0000001) ? 0 : item.estimate);
		},
		keepAll: false
	});

	$("#active-order-list").irTable({
		header: ["Id", "Strategy", "Context", "Pair"," Rate", "Amount", "Creation Time", "Timeout"],
		makeRow: function(item) {
			var content = "<tr class=\"context-" + item.context
	    			+ " strategy-" + item.strategy
	    			+ " currency-" + item.initialCurrency
	    			+ " currency-" + item.finalCurrency + "\">";
	    	content += "<td>" + item.id + "</td>";
	    	content += "<td data-mouseover=\"tr.strategy-" + item.strategy + "\">" + item.strategy + "</td>";
	    	content += "<td data-mouseover=\"tr.context-" + item.context + "\">" + item.context + "</td>";
	    	content += "<td data-mouseover=\"tr.currency-" + item.initialCurrency
	    			+ ".currency-" + item.finalCurrency + "\">"
	    			+ item.initialCurrency + "/" + item.finalCurrency + "</td>";
	    	content += "<td class=\"update-rate-" + item.initialCurrency + "-" + item.finalCurrency + "\" data-rate=\"" + item.rate + "\">" + item.rate + "</td>";
	    	content += "<td>" + item.amount + "</td>";
	    	content += "<td>" + View.toDate(item.creationTime) + "</td>";
	    	content += "<td>" + item.timeout + "s</td>";
	    	content += "</tr>";
	    	return content;
		},
		keepAll: false
	});

	$("#order-list").irTable({
		header: ["Type", "Time", "Id", "Strategy", "Context", "Pair"," Rate", "Amount", "Message"],
		makeRow: function(item) {
			var content = "<tr class=\"context-" + item.context
    				+ " strategy-" + item.strategy
    				+ " currency-" + item.initialCurrency
    				+ " currency-" + item.finalCurrency + "\">";
    		switch (item.type) {
    		case "cancel":
    			content += "<td><i class=\"fa fa-times\" aria-hidden=\"true\"></i> Cancel</td>";
    			break;
    		case "failed":
    			content += "<td><i class=\"fa fa-times\" aria-hidden=\"true\"></i> Failed</td>";
    			break;
    		case "timeout":
    			content += "<td><i class=\"fa fa-hourglass\" aria-hidden=\"true\"></i> Timeout</td>";
    			break;
    		case "partial":
    			content += "<td><i class=\"fa fa-star-half-o\" aria-hidden=\"true\"></i> Partial</td>";
    			break;
    		case "proceed":
    			content += "<td><i class=\"fa fa-star\" aria-hidden=\"true\"></i> Proceed</td>";
    			break;
    		case "place":
    			content += "<td><i class=\"fa fa-star-o\" aria-hidden=\"true\"></i> Place</td>";
    			break;
    		default:
    			content += "<td>" + item.type + "</td>";
    		}
	    	content += "<td>" + View.toDate(item.timestamp) + "</td>";
	    	content += "<td>" + item.id + "</td>";
	    	content += "<td data-mouseover=\"tr.strategy-" + item.strategy + "\">" + item.strategy + "</td>";
	    	content += "<td data-mouseover=\"tr.context-" + item.context + "\">" + item.context + "</td>";
	    	content += "<td data-mouseover=\"tr.currency-" + item.initialCurrency
	    			+ ".currency-" + item.finalCurrency + "\">"
	    			+ item.initialCurrency + "/" + item.finalCurrency + "</td>";
	    	content += "<td>" + item.rate + "</td>";
	    	content += "<td>" + item.amount + "</td>";
	    	content += "<td>" + item.message + "</td>";
	    	content += "</tr>";
	    	return content;
		},
		keepAll: true
	});

	$("#pair-list").irTable({
		header: ["", "Pair", "Decimal", "Decimal (Orders)", "Fee", "Boundaries", "Rate"],
		makeRow: function(item) {
			var content = "<tr class=\"currency-" + item.initialCurrency
    				+ " currency-" + item.finalCurrency + "\">";
	    	content += "<td><button class=\"btn btn-default btn-xs\" onclick=\"javascript:Trader.addMonitor(" + exchange.getId() + ", '" + item.initialCurrency + "/" + item.finalCurrency + "');\">";
	    	content += "<i class=\"fa fa-area-chart\" aria-hidden=\"true\"></i> Monitor</button></td>";
	    	content += "<td data-mouseover=\"tr.currency-" + item.initialCurrency
	    			+ ".currency-" + item.finalCurrency + "\">"
	    			+ item.initialCurrency + "/" + item.finalCurrency + "</td>";
	    	content += "<td>" + item.decimal + "</td>";
	    	content += "<td>" + item.decimalOrder + "</td>";
	    	content += "<td>" + item.boundaries + "</td>";
	    	content += "<td>" + item.fee + "</td>";
	    	content += "<td>-</td>";
	    	content += "</tr>";
			return content;
		},
		isMatch: function(item1, item2) {
			return (item1.initialCurrency == item2.initialCurrency
					&& item1.finalCurrency == item2.finalCurrency);
		},
		updateRow: function(row, item) {
			var cell = $(row).children("td:nth(6)");

			$(cell).removeClass("increase decrease");
			var className = "";
			var previousRate = parseFloat($(cell).text());
			if (item.rate > previousRate) {
				className = "increase";
			}
			else if (item.rate < previousRate) {
				className = "decrease";
			}

			// Update the active orders with the new rate if any
			if (item.rate != previousRate) {
				$(cell).addClass(className);
				var orderRateSelector = "#active-order-list .update-rate-" + item.initialCurrency + "-" + item.finalCurrency;
				$(orderRateSelector).each(function() {
					var orderRate = $(this).attr("data-rate");
					var diffPercent = ((1 - orderRate / item.rate) * 100);
					$(this).html(orderRate + " (<span class=\"" + className + "\">" + item.rate + "</span> " + diffPercent.toFixed(1) + "%)");
				});
			}

			$(cell).text(item.rate);
		},
		keepAll: false
	});

	$("#pair-list").add("#order-list").add("#active-order-list").add("#balance-list").on("mouseover", "td", function(e) {
		var elt = e.target;
		$(".mouseovermark").removeClass("mouseovermark");
		if ($(elt).attr("data-mouseover")) {
			var selector = $(elt).attr("data-mouseover");
			$(selector).addClass("mouseovermark");
		}
		e.stopPropagation();
	});

	$("#pair-monitor").irPlot();
};

View.prototype.setStrategy = function (strategy) {
	$("#page-wrapper").html(

		"<br/><div class=\"row\">"
			+ "<div class=\"col-lg-12 col-md-6\">"
	            + "<div class=\"panel panel-primary\">"
	                + "<div class=\"panel-heading\">"
	                	+ "<div class=\"row\">"
	                		+ "<div class=\"col-lg-12\">"
	                			+ "<h1>" + strategy.getName() + "</h1>"
	                		+ "</div>"
	                	+ "</div>"
	                	+ "<div class=\"row\">"
	                		+ "<div class=\"col-lg-4\">"
	                			+ "<div id=\"strategy-estimate\"></div>"
	                		+ "</div>"
	                	+ "</div>"
	                + "</div>"
	            + "</div>"
	         + "</div>"
		+ "</div>"

		+ "<div class=\"row\">"
			+ "<div class=\"col-lg-6\">"
				+ "<div class=\"panel panel-default\">"
					+ "<div class=\"panel-heading\">"
		            	+ "<i class=\"fa fa-usd\"></i> Profit"
		            + "</div>"
		            + "<div class=\"panel-body\">"
		            	+ "<div id=\"strategy-profit\" class=\"table-responsive\"></div>"
		            	+ "<div id=\"strategy-stats\"></div>"
		            + "</div>"
				+ "</div>"
			+ "</div>"
		+ "</div>"
	);

	$("#strategy-profit").irTable({
		header: ["Currency", "Funds", "Reserved", "~USD"],
		makeRow: function(item) {
			var content = "<tr class=\"currency-" + item.key + "\">";
	    	content += "<td data-mouseover=\"tr.currency-" + item.key + "\">" + item.key + "</td>";
	    	content += "<td>" + item.amount + "</td>";
	    	content += "<td>" + item.reserve + "</td>";
	    	content += "<td>" + item.estimate + "</td>";
	    	content += "</tr>";
	    	return content;
		},
		isMatch: function(item1, item2) {
			return (item1.key == item2.key);
		},
		updateRow: function(row, item) {
			$(row).children("td:nth(1)").text(item.amount);
			$(row).children("td:nth(2)").text(item.reserve);
			$(row).children("td:nth(3)").text(item.estimate);
		},
		keepAll: false
	});

};

View.toElapsedDate = function (timestamp, curTimestamp) {

	if (typeof curTimestamp === "undefined") {
		var curDate = new Date();
		curTimestamp = curDate.getTime();
	}

	var d = new Date(timestamp);
	
	var diffSeconds = (curTimestamp - d.getTime()) / 1000;

	if (diffSeconds < 2) {
		return "now";
	}
	else if (diffSeconds < 60) {
		return Math.floor(diffSeconds) + " seconds ago";
	}
	else if (diffSeconds < 60 * 2) {
		return "1 minute ago";
	}
	else if (diffSeconds < 60 * 60) {
		return Math.floor(diffSeconds / 60) + " minutes ago";
	}
	else if (diffSeconds < 60 * 60 * 2) {
		return "1 hour ago";
	}
	else if (diffSeconds < 60 * 60 * 24) {
		return Math.floor(diffSeconds / (60 * 60)) + " hours ago";
	}
	else if (diffSeconds < 60 * 60 * 24 * 2) {
		return "yesterday";
	}
	else {
		return Math.floor(diffSeconds / (60 * 60 * 24)) + " days ago";
	}

	return d.toLocaleString();
};

View.toDate = function (timestamp) {
	var d = new Date(timestamp);
	return d.toLocaleString();
};

View.prototype.updateManager = function (data) {
	var curTime = data["time"];
	$("#manager-memory").irPlot("update", "memory", [[curTime, data["memoryVirtualMemoryCurrent"]]]);
	$("#manager-memory").irPlot("update", "alloc", [[curTime, data["memoryAllocCurrent"]]]);
	$("#manager-memory").irPlot("update", "alloc peak", [[curTime, data["memoryAllocPeak"]]]);
	$("#manager-table").irTable("update", [
		["Version", data["version"]],
		["Time", View.toDate(curTime)],
		["Started", View.toElapsedDate(data["startTime"], curTime)]
	]);
};

View.prototype.updateThreads = function (data) {
	$("#manager-threads").irTable("update", data);
};

View.prototype.updateTraces = function (data) {

	var content = "";
	if (data.warnings.list.length > 0) {
		content += "<a href=\"#manager-traces\" class=\"list-group-item\">";
		content += "<i class=\"fa fa-exclamation-triangle\"></i> " + data.warnings.list.length + " warning(s)";
		content += "<span class=\"pull-right text-muted small\"><em>" + View.toElapsedDate(data.warnings.timestamp) + "</em></span>";
		content += "</a>";

		$("#manager-traces").irTable("update", data.warnings.list);
	}

	if (data.errors.list.length > 0) {
		content += "<a href=\"#manager-traces\" class=\"list-group-item\">";
		content += "<i class=\"fa fa-times-circle\"></i> " + data.errors.list.length + " error(s)";
		content += "<span class=\"pull-right text-muted small\"><em>" + View.toElapsedDate(data.errors.timestamp) + "</em></span>";
		content += "</a>";

		$("#manager-traces").irTable("update", data.errors.list);
	}

	$("#manager-notifications").html(content);
};

View.prototype.updateActiveOrderList = function (data) {
	$("#active-order-list").irTable("update", data);
	// Update the active order info
	{
		var infoContent = "<span class=\"header-metrics\">" + data.length + "</span> active order" + ((data.length > 1) ? "s" : "");
		$("#active-orders-metrics").html(infoContent);
	}
};

View.prototype.updateOrderList = function (data) {
	$("#order-list").irTable("update", data);
};

View.prototype.updateStrategyProfit = function (data) {

	var profitList = [];

	// Loop through the various exchanges
	$("#strategy-profit").empty();
	for (var i in data) {
		var profit =  data[i].profit;
		var total = profit.total;
		var estimateCurrency = profit.estimate;
		delete profit.total;
		delete profit.estimate;

		var container = $("<div/>");
		$(container).irTable({
			header: ["Currency", "Profit", "~" + estimateCurrency],
			makeRow: function(item) {
				var content = "<tr class=\"currency-" + item.key + "\">";
	    		content += "<td data-mouseover=\"tr.currency-" + item.key + "\">" + item.key + "</td>";
	    		content += "<td>" + ((item.amount > 0) ? "+" : "") + item.amount + "</td>";
	    		content += "<td>" + item.estimate + "</td>";
	    		content += "</tr>";
	    		return content;
			}
		});
		$(container).irTable("update", profit);
		$("#strategy-profit").append(container);

		profitList.push(((total.estimate > 0) ? "+" : "")
				+ parseFloat(Math.round(total.estimate * 100) / 100).toFixed(2) + " " + estimateCurrency);

		$("#strategy-stats").text("Success: " + data[i].nbSuccess + ", Failure Timeout:"
				+ data[i].nbFailedTimeout + ", Failure Place Order:" + data[i].nbFailedPlaceOrder);
	}

	// Update the total
	$("#strategy-estimate").html("<span class=\"header-metrics\">Profit: "
			+ profitList.join(",") + "</span>");
};

View.prototype.updateBalance = function (data) {
	var total = data.total;
	var estimateCurrency = data.estimate;
	delete data.total;
	delete data.estimate;
	$("#balance-list").irTable("update", data);

	// Update the total
	var diffPrecent = (total.estimate - total.initial) / total.initial * 100;
	$("#balance-estimate").html("<span class=\"header-metrics\">" + parseFloat(Math.round(total.estimate * 100) / 100).toFixed(2)
			+ " USD</span> (" + ((diffPrecent > 0) ? "+" : "") + parseFloat(Math.round(diffPrecent * 100) / 100).toFixed(2) + "%)");

	// Update the graph
	var d = new Date();
	var data = [[d.getTime(), total.estimate]];
	$("#balance-graph").irPlot("update", "Estimate", data);
	var data = [[d.getTime(), total.initial]];
	$("#balance-graph").irPlot("update", "Initial Value", data);
};

View.prototype.updateInitialBalance = function (data) {

	var total = data.total;

	// Update the total
	var diffPrecent = (total.estimate - total.initial) / total.initial * 100;
	$("#balance-initial-estimate").html("Initial Value: <span class=\"header-metrics\">" + parseFloat(Math.round(total.estimate * 100) / 100).toFixed(2)
			+ " USD</span>");

	// Update the graph
	var d = new Date();
	var data = [[d.getTime(), total.estimate]];
	$("#balance-graph").irPlot("update", "Initial Value w/current Estimate", data);
};

View.prototype.updateTransactionsList = function (data) {
	$("#pair-list").irTable("update", data);
	// Update the active pairs info
	{
		var infoContent = "<span class=\"header-metrics\">" + data.length + "</span> active pair" + ((data.length > 1) ? "s" : "");
		$("#active-pairs-metrics").html(infoContent);
	}
};

View.prototype.strategyClear = function () {
	$("#strategy-menu").empty();
};

View.prototype.strategyAdd = function (strategy) {
	// Add the item to the menu
	{
		var menuItem = $("<li/>");
		$(menuItem).append("<a onclick=\"javascript:Trader.changeView('strategy', {index:" + strategy.getId() + "});\">" + strategy.getName() + "</a>");
		$("#strategy-menu").append(menuItem);
	}
};

View.prototype.exchangeClear = function () {
	$("#exchange-status").empty();
	$("#exchange-menu").empty();
};

View.prototype.updatePairMonitor = function (label, data) {
	$("#pair-monitor").irPlot("update", label, data);
};

View.prototype.exchangeAdd = function (exchange) {

	var colorClass = "";
	var text = "";

	if (exchange.isConnected()) {
		colorClass = "panel-green";
		text = "Connected since " + View.toDate(exchange.getTimestampConnected());
	}
	else if (exchange.isConnecting()) {
		colorClass = "panel-yellow";
		text = "Connecting...";
	}
	else if (exchange.isDisconnecting()) {
		colorClass = "panel-yellow";
		text = "Disconnecting...";
	}
	else if (exchange.isDisconnected()) {
		colorClass = "panel-red";
		text = "Disconnected<br/>Last connection: " + View.toDate(exchange.getTimestampConnected());
	}
	else {
		alert("Error, unkown exchange status");
	}
	
	var d = new Date();
	var timeDelta = exchange.getTimestamp() - d.getTime();

	var content = "<div class=\"col-lg-3 col-md-6\">"
		+ "<div class=\"panel " + colorClass + " exchange-status-" + exchange.getId() + "\">"
			+ "<div class=\"panel-heading\" onclick=\"javascript:Trader.changeView('exchange', {index:" + exchange.getId() + "});\">"
				+ "<div class=\"row\">"
					+ "<div class=\"col-xs-3\">"
						+ "<i class=\"fa fa-server fa-5x\"></i>"
					+ "</div>"
					+ "<div class=\"col-xs-9 text-right\">"
						+ "<div style=\"white-space: nowrap;\" class=\"huge\">" + exchange.getName() + "</div>"
						+ "<div style=\"white-space: nowrap;\">Time delta: " + ((timeDelta > 0) ? "+" : "") + (timeDelta / 1000) + "s</div>"
						+ "<div style=\"white-space: nowrap;\">" + text + "</div>"
					+ "</div>"
				+ "</div>"
			+ "</div>"
			+ "<a href=\"#\" onclick=\"View.showExchangeConfig(this, " + exchange.getId() + ");\">"
				+ "<div class=\"panel-footer\">"
					+ "<span class=\"pull-left\">View Details</span>"
					+ "<span class=\"pull-right\"><i class=\"fa fa-arrow-circle-right\"></i></span>"
					+ "<div class=\"clearfix\"></div>"
				+ "</div>"
			+ "</a>"
		+ "</div>"
	+ "</div>";

	$("#exchange-status").append(content);

	// Add the item to the menu
	{
		var menuItem = $("<li/>");
		$(menuItem).append("<a onclick=\"javascript:Trader.changeView('exchange', {index:" + exchange.getId() + "});\">" + exchange.getName() + "</a>");
		$("#exchange-menu").append(menuItem);
	}
}
