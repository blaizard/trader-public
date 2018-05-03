function Strategy(name, id) {
	this.name = name;
	this.id = id;
};

Strategy.prototype.getName = function () {
	return this.name;
};

Strategy.prototype.getId = function () {
	return this.id;
};
