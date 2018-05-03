irRequire.track = {};
irRequire.total = 0;
irRequire.done = 0;
irRequire.h = (name, url) => {

	// Create the progress bar
	if (!irRequire.progress) {
		irRequire.progress = document.createElement("div");
		document.getElementsByTagName("body")[0].appendChild(irRequire.progress);
	}

	irRequire.progress.className = "irRequire-progress";
	irRequire.track[name] = irRequire.track[name] || [];
	if (url) {
		irRequire.track[name].push(url);
		++irRequire.total;
	}
	else
	{
		irRequire.done += irRequire.track[name].length;
		delete irRequire.track[name];
	}

	if (irRequire.total == irRequire.done) {
		irRequire.total = 0;
		irRequire.done = 0;
		irRequire.progress.style.width = "100%";
		irRequire.progress.className = "irRequire-progress done";
	}
	else
	{
		var completed = ((irRequire.done + 0.1) / (irRequire.total + 0.1) * 100);
		irRequire.progress.style.width = completed + "%";
	}
};
