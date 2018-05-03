function irAjax(a, b)
{
	var irAjaxObj = function(type, url) {
		this.onSuccess = [];
		this.onError = [];
		this.onComplete = [];
		this.data = "";

		var xhr;
		if (typeof XMLHttpRequest !== "undefined") {
			xhr = new XMLHttpRequest();
		}
		// If not available look for one that is available
		else {
			var versions = ["MSXML2.XmlHttp.5.0", 
					"MSXML2.XmlHttp.4.0",
					"MSXML2.XmlHttp.3.0", 
					"MSXML2.XmlHttp.2.0",
					"Microsoft.XmlHttp"];
			for (var i = 0, len = versions.length; i < len; i++) {
				try {
					xhr = new ActiveXObject(versions[i]);
					break;
				}
				catch(e) {
				}
			}
		}

		var obj = this;
		xhr.onreadystatechange = function() {
			if(xhr.readyState === 4) {
				if (xhr.status == 200 || (xhr.status == 0 && xhr.responseText)) {
					obj.data = xhr.responseText;
					for (var i in obj.onSuccess) {
						obj.onSuccess[i].call(obj, obj.data);
					}
				}
				else {
					for (var i in obj.onError) {
						obj.onError[i].call(obj, "Unable to load '" + url + "'");
					}
				}
				for (var i in obj.onComplete) {
					obj.onComplete[i].call(obj);
				}
			}
		}

		xhr.open(type, url, true);
		xhr.send();
	}

	irAjaxObj.prototype.success = function(callback) {
		this.onSuccess.push(callback);
		return this;
	};

	irAjaxObj.prototype.error = function(callback) {
		this.onError.push(callback);
		return this;
	};

	irAjaxObj.prototype.complete = function(callback) {
		this.onComplete.push(callback);
		return this;
	};

	if (typeof b === "undefined") {
		return new irAjaxObj("GET", a);
	}
	return new irAjaxObj(a, b);
}

function irAjaxJson(a, b)
{
	return irAjax(a, b).success(function(data) {
		this.data = JSON.parse(data);
	});
}
