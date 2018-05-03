Vue.component('balance', {
	template: '<div class="balance">'
			+ '<div class="charts">'
			+ '<div class="history-chart"><chart :data-list="dataList" :max-data="25"></chart></div>'
			+ '<div class="pie-chart"><canvas class="pie-chart-container"></canvas></div>'
			+ '</div>'
			+ '<table>'
			+ '<thead><tr>'
			+ '<th></th><th>Currency</th><th>Funds</th><th v-if="isReserved">Reserved</th><th>~{{ estimateCurrency }}</th>'
			+ '</tr></thead>'
			+ '<tbody>'
			+ '<tr v-for="entry in balanceData" v-bind:class="entry[1]">'
			+ '<td><div class="color" :style="{\'background-color\': entry[0]}">&nbsp;</div></td>'
			+ '<td v-for="i in (isReserved) ? [1, 2, 3, 4] : [1, 2, 4]">'
			+ '{{ entry[i] }}'
			+ '</td>'
			+ '</tr>'
			+ '</tbody>'
			+ '</table>'
			+ '</div>',
	props: ['balance', 'type', 'initialBalance'],
	data: function() {
		var dataList = [{
			title: "Estimate",
			type: 'line',
			timeseries: this.balance.timeseries,
			color: ['#3cb44b', '#3cb44b']
		}];

		if (this.initialBalance) {
			dataList.push({
				title: "Initial Estimate",
				type: 'dashLine',
				timeseries: this.initialBalance.timeseries,
				color: ['rgba(0, 0, 0, 0.3)', '#000']
			});
		}

		if (this.type != "profit") {
			dataList.push({
				title: "Initial Value",
				type: 'fill',
				horizontal: this.balance.getInitialEstimate(),
				color: ['rgba(255, 0, 0, 0.2)', 'rgba(255, 0, 0, 0.1)']
			});
		}
		return {
			pieChart: null,
			pieChartConfig: null, 
			colorList: [
				'#e6194b', '#3cb44b', '#ffe119', '#0082c8', '#f58231', '#911eb4', '#46f0f0', '#f032e6',
				'#d2f53c', '#fabebe', '#008080', '#e6beff', '#aa6e28', '#fffac8', '#800000', '#aaffc3',
				'#808000', '#ffd8b1', '#000080', '#808080', '#ffffff', '#000000'
			],
			dataList: dataList
		};
	},
	mounted() {
		// Update the pie chart
		{
			this.pieChartConfig = {
				type: 'pie',
				data: {
					datasets: [],
					labels: []
				},
				options: {
					responsive: true,
					legend: {
						display: false
					},
					tooltips: {
						custom: (tooltipModel) => {
							$(this.$el).find("tr").removeClass("selected");
							for (var index in tooltipModel.dataPoints) {
								var i = tooltipModel.dataPoints[index].index;
								var currency = this.pieChartConfig.data.labels[i];
								$(this.$el).find("tr." + currency).addClass("selected");
							}
							tooltipModel.opacity = 0;
						}
					},
					title: {
						display: true
					},
					animation: {
						duration: 0
					}
				}
			};

			var ctx = this.$el.getElementsByClassName("pie-chart-container")[0].getContext("2d");
			this.pieChart = new Chart(ctx, this.pieChartConfig);
			this.updatePieChart();
		}
	},
	updated() {
		this.updatePieChart();
	},
	methods: {
		updatePieChart() {
			this.pieChartConfig.data.datasets = [{
					data: [],
					backgroundColor: this.colorList
			}];
			this.pieChartConfig.data.labels = [];
			this.balance.each((currency, value) => {
				this.pieChartConfig.data.datasets[0].data.push(Math.abs(value["estimate"]));
				this.pieChartConfig.data.labels.push(currency);
			});
			this.pieChartConfig.options.title.text = "Estimated value: "
				+ this.balance.getEstimateString();
			this.pieChart.update();
		}
	},
	computed: {
		estimateCurrency() {
			return this.balance.getEstimateCurrency();
		},
		balanceData() {
			var balanceData = [];
			this.balance.each((currency, value, i) => {
				balanceData.push([this.colorList[i % this.colorList.length], currency, value["amount"], value["reserve"], Math.round(value["estimate"] * 10) / 10]);
			});
			return balanceData;
		},
		isReserved() {
			return (this.type != "profit");
		}
	}
});

