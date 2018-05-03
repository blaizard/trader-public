function Exchange(name, status, id, timestampDelta, timestampConnected) {
	this.name = name;
	this.status = status;
	this.id = id;
	this.timestampDelta = timestampDelta;
	this.timestampConnected = timestampConnected;
	this.connectionError = 0;
};

Exchange.prototype.isConnected = function () {
	return (this.status == 3);
};

Exchange.prototype.isConnecting = function () {
	return (this.status == 2);
};

Exchange.prototype.isDisconnecting = function () {
	return (this.status == 1);
};

Exchange.prototype.isDisconnected = function () {
	return (this.status == 0);
};

Exchange.prototype.isConnectionError = function () {
	return (this.connectionError > 0);
};

Exchange.prototype.getTimestamp = function () {
	var d = new Date();
	return d.getTime() - this.timestampDelta;
};

Exchange.prototype.getTimestampConnected = function () {
	return this.timestampConnected;
};

Exchange.prototype.getName = function () {
	return this.name;
};

Exchange.prototype.getId = function () {
	return this.id;
};

Exchange.prototype.setConnectionError = function () {
	this.connectionError++;
};
