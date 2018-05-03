/**
 * jQuery Module Template
 *
 * This template is used for jQuery modules.
 *
 */

(function($) {
	/**
	 * \brief .\n
	 * Auto-load the irTable modules for the tags with a data-irTable field.\n
	 * 
	 * \alias jQuery.irTable
	 *
	 * \param {String|Array} [action] The action to be passed to the function. If the instance is not created,
	 * \a action can be an \see Array that will be considered as the \a options.
	 * Otherwise \a action must be a \see String with the following value:
	 * \li \b create - Creates the object and associate it to a selector. \code $("#test").irTable("create"); \endcode
	 *
	 * \param {Array} [options] The options to be passed to the object during its creation.
	 * See \see $.fn.irTable.defaults a the complete list.
	 *
	 * \return {jQuery}
	 */
	$.fn.irTable = function(arg, data) {
		var retval;
		// Go through each objects
		$(this).each(function() {
			retval = $().irTable.x.call(this, arg, data);
		});
		// Make it chainable, or return the value if any
		return (typeof retval === "undefined") ? $(this) : retval;
	};

	/**
	 * This function handles a single object.
	 * \private
	 */
	$.fn.irTable.x = function(arg, data) {
		// Load the default options
		var options = $.fn.irTable.defaults;

		// --- Deal with the actions / options ---
		// Set the default action
		var action = "create";
		// Deal with the action argument if it has been set
		if (typeof arg === "string") {
			action = arg;
		}
		// If the module is already created and the action is not create, load its options
		if (action != "create" && $(this).data("irTable")) {
			options = $(this).data("irTable");
		}
		// If the first argument is an object, this means options have
		// been passed to the function. Merge them recursively with the
		// default options.
		if (typeof arg === "object") {
			options = $.extend(true, {}, options, arg);
		}
		// Store the options to the module
		$(this).data("irTable", options);

		// Handle the different actions
		switch (action) {
		// Create action
		case "create":
			$.fn.irTable.create.call(this);
			break;
		// Update the content of the table with new values
		case "update":
			$.fn.irTable.update.call(this, data);
			break;
		};
	};

	$.fn.irTable.create = function() {
		// Reset the data
		var options = $(this).data("irTable");
		options.data = [];
		$(this).data("irTable", options);

		var table = $("<table/>", {
			class: "table table-bordered table-hover table-striped"
		});
		var thead = $("<thead/>");
		var tr = $("<tr/>");
		for (var i in options.header) {
			var th = $("<th/>");
			$(th).text(options.header[i]);
			$(tr).append(th);
		}
		$(thead).append(tr);
		$(table).append(thead);
		$(table).append($("<tbody/>"));
		$(this).html(table);
	};

	$.fn.irTable.isMatch = function(item) {
		var options = $(this).data("irTable");

	    for (var i in options.data) {
	    	if (options.isMatch.call(this, item, options.data[i])) {
	    		return i;
	    	}
	    }

		return false;
	};

	$.fn.irTable.update = function(data) {
		var options = $(this).data("irTable");
		var insertSelector = $(this).find("tbody:first");

		var list = data;
		// If data is an object trasnform it into an array
		if (!$.isArray(data)) {
			list = [];
			for (var key in data) {
				var elt = data[key];
				elt["key"] = key;
				list.push(elt);
			}
			list = list.sort(function(a, b){
				(a.key < b.key) ? -1 : 1;
			});
		}

		// Contains the list of items that have been updated
		var updatedList = [];
		for (var i in options.data) {
			updatedList[i] = false;
		}

		// Update the items
		var previousMatch = -1;
	    for (var i = list.length - 1; i >= 0; i--) {

	    	var item = list[i];
	    	var match = $.fn.irTable.isMatch.call(this, item);

	    	// Look for a match in the list
	    	if (match === false) {
		    	var tr = options.makeRow.call(this, item);
		    	// Prepend the new row or insert it after the previous match (if any)
		    	if (previousMatch == -1) {
			    	$(insertSelector).prepend(tr);
		    		options.data.unshift(item);
		    		updatedList.unshift(true);
	    		}
	    		else {
	    			$(insertSelector).find("tr:eq(" + previousMatch + ")").before(tr);
	    			options.data.splice(previousMatch, 0, item);
	    			updatedList.splice(previousMatch, 0, true);
	    		}
	    	}
	    	else {
	    		// Update the current row if needed
	    		var tr = $(insertSelector).find("tr:eq(" + match + ")");
	    		options.updateRow.call(this, tr, item);
	    		// Set the current row as updated
	    		if (match < updatedList.length) {
	    			updatedList[match] = true;
	    			previousMatch = match;
	    		}
	    	}
	    }

	    // Remove lines that have not been updated
	    if (!options.keepAll) {
	    	var index = 0;
	    	for (var i in updatedList) {
	    		if (updatedList[i]) {
	    			index++;
	    		}
	    		else {
	    			$(insertSelector).find("tr:eq(" + index + ")").remove();
	    			options.data.splice(index, 1);
	    		}
	    	}
	    }

	    // Save the data
	    $(this).data("irTable", options);
	};

	/**
	 * \brief Default options, can be overwritten. These options are used to customize the object.
	 * Change default values:
	 * \code $().irTable.defaults.theme = "aqua"; \endcode
	 * \type Array
	 */
	$.fn.irTable.defaults = {
		/**
		 * Specifies a custom theme for this element.
		 * By default the irTable class is assigned to this element, this theme
		 * is an additional class to be added to the element.
		 * \type String
		 */
		theme: "",
		header: [],
		/**
		 * Create a row from the item and return it
		 */
		makeRow: function(item) {
			var tr = $("<tr/>");
			for (var i in item) {
				var td = $("<td/>");
				$(td).text(item[i]);
				$(tr).append(td);
			}
			return tr;
		},
		updateRow: function(tr, item) {},
		/**
		 * Look for a match between 2 items. The match is expected
		 * to be unique within the table.
		 */
		isMatch: function(item1, item2) {
			return (JSON.stringify(item1) == JSON.stringify(item2));
		},
		keepAll: false
	};
})(jQuery);
