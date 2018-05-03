Vue.component('chart', {
	template: '<canvas class="chart"></canvas>',
	props: ['dataList', 'maxData'],
	data: function() {
		return {
			refreshPeriodMs: 1000,
			periodMs: 1 * 3600 * 1000, // Last hour
			config: {
				type: 'line',
				data: {
					datasets: []
				},
				options: {
					scales: {
						xAxes: [{
							type: "time",
							time : {
								tooltipFormat: "YYYY-MM-DD HH:mm:ss"
							},
							ticks: {
								maxRotation: 0,
								minRotation: 0
							}
						}]
					},
					tooltips: {
						mode: "x",
						intersect: false
					},
					animation: {
						duration: 0
					},
					legend: {
						display: true,
						labels: {
							boxWidth: 15
						}
					}
				}
			},
			chartObj: null,
			timeout: [],
			dataWatch: [],
			colorList: [
				["rgba(255, 0, 0, 0.3)", "#e24825"], // red
				["rgba(34, 140, 34, 0.3)", "#5fba7d"], // green
				["rgba(0, 0, 255, 0.3)", "#1e90ff"], // blue
				["rgba(255, 150, 50, 0.3)", "#e67e22"], // yellow
				["rgba(0, 0, 0, 0.3)", "#000000"] // black
			]
		};
	},
	mounted() {
		// Generate the data pairs (x/y)
		for (var i in this.dataList) {
			var color = (this.dataList[i].color) ? this.dataList[i].color : this.colorList[i % (this.colorList.length)];
			var data = {
				label: this.dataList[i].title,
				data: [],
				fill: false,
				radius: 1,
				borderColor: color[0],
				backgroundColor: color[1],
			};

			switch (this.dataList[i].type)
			{
			case 'line':
				break;
			case 'dashLine':
				data.borderDash = [5, 5];
				data.backgroundColor = "rgba(255, 255, 255, 0)";
				data.radius = 0;
				break;
			case 'fill':
				data.fill = true;
				break;
			case 'rate':
				data.steppedLine = "before";
				break;
			}

			this.config.data.datasets.push(data);
			this.$set(this.dataWatch, i, this.dataList[i]);
			this.timeout[i] = null;
			if (this.dataWatch[i].url)
			{
				this.fetchData(i, this.dataWatch[i].url);
			}
		}

		// Generate the graph
		this.chartObj = new Chart(this.$el, this.config);
	},
	beforeDestroy() {
		for (var i in this.timeout) {
			clearInterval(this.timeout[i]);
		}
	},
	watch: {
		dataWatch: {
			handler: function (val, oldVal) {
				this.redraw();
			},
			deep: true
		}
	},
	methods: {
		/**
		 * Redraw the graph
		 */
		redraw() {
			var lastestX = Number.MIN_VALUE;
			var firstX = Number.MAX_VALUE;

			// Copy the data
			for (var i in this.dataWatch) {
				var data = [];
				if (this.dataWatch[i].timeseries) {
					this.dataWatch[i].timeseries.eachAverage((t, d) => {
						data.push({x: t, y: d});
					}, this.maxData);
				}
				if (data.length)
				{
					lastestX = (data.length) ? Math.max(lastestX, data[data.length - 1].x) : lastestX;
					firstX = (data.length) ? Math.min(firstX, data[0].x) : firstX;
				}
				this.config.data.datasets[i].data = data;
			}

			// Add a point completly on the left and right of the graph
			for (var i in this.dataWatch) {

				var firstY, lastestY;
				if (this.config.data.datasets[i].data.length) {
					firstY = this.config.data.datasets[i].data[0].y;
					lastestY = this.config.data.datasets[i].data[this.config.data.datasets[i].data.length - 1].y;

				}
				else if (this.dataWatch[i].horizontal) {
					firstY = this.dataWatch[i].horizontal;
					lastestY = this.dataWatch[i].horizontal;
				}
				else {
					continue;
				}
				this.config.data.datasets[i].data.unshift({x: firstX, y: firstY});
				this.config.data.datasets[i].data.push({x: lastestX, y: lastestY});
			}

			this.chartObj.update();
		},
		/**
		 * Gather new data
		 */
		fetchData(id, url) {
			var d = new Date();
			var lastSeriesTimestamp = this.dataWatch[id].timeseries.getEndTimestamp();
			var lastTimestamp = (lastSeriesTimestamp) ? lastSeriesTimestamp : (d.getTime() - this.periodMs);
			var updatedUrl = url + "/" + d.getTime() + "/" + (lastTimestamp + 1);

			irAjaxJson(updatedUrl).success((data) => {
				// If this is the first time for redraw
				var forceRedraw = this.dataWatch[id].timeseries.empty();
				// Add the data and update the synchronization flag
				this.dataWatch[id].timeseries.add(data.list, /*ascending*/false);
			}).complete(() => {
				this.timeout[id] = setTimeout(() => {
					this.fetchData(id, url);
				}, this.refreshPeriodMs);
			});
		}
	}
});
