var App = function() {
	this.manager = new Manager();
	this.instance = null;
};

App.prototype.run = function() {
	irRequire(["Vue",
			"VueRouter",
			"jQuery",
			"VueComponent('exchange-ticker')",
			"VueComponent('balance')",
			"VueComponent('strategy-ticker')",
			"VueComponent('chart')",
			"VueComponent('metric')",
			"VueComponent('exchange-pairs')",
			"VueComponent('properties')",
			"VueComponent('exchange-orders')",
			"VueComponent('traces')",
			"VueComponent('operations')",
			"VueComponent('threads')"], () => {
		this.instance = new Vue({
			el: "#app",
			components: {'side-menu': this.createMenu(), 'app-version': this.createVersion()},
			router: this.createRouter(),
			data: this.manager.data
		});
	});
};

App.prototype.createMenu = function() {
	const viewMenu = Vue.extend({
		template: '<div>'
				+ '<li><a href="new/index.html#/">Overview</a></li>'
				+ '<li><a href="new/index.html#/chart">Chart</a></li>'
				+ '<li><span class="menu-title">Exchange(s)</span>'
				+ '<ul v-for="exchange in exchangeList">'
				+ '<li><a :href="\'new/index.html#/exchange/\' + exchange.getId()">{{ exchange.getName() }}</a></li>'
				+ '</ul>'
				+ '</li>'
				+ '<li><span class="menu-title">Strategy(s)</span>'
				+ '<ul v-for="strategy in strategyList">'
				+ '<li><a :href="\'new/index.html#/strategy/\' + strategy.getId()">{{ strategy.getName() }}</a></li>'
				+ '</ul>'
				+ '</li>'
				+ '</div>',
		props: ["exchangeList", "strategyList"]
	});
	return viewMenu;
};

App.prototype.createVersion = function() {
	const viewVersion = Vue.extend({
		template: '<span v-if="managerInfo.version">v.{{ this.managerInfo.version }}</span>',
		props: ["managerInfo"]
	});
	return viewVersion;
};

App.prototype.createRouter = function() {

	const viewDashboard = {
		template: '<div>'
				+ '<h1 v-if="exchangeList.length">Exchange(s)</h1>'
				+ '<span v-for="exchange in exchangeList">'
				+ '<exchange-ticker :exchange="exchange"></exchange-ticker>'
				+ '</span>'
				+ '<h1 v-if="strategyList.length">Strategy List</h1>'
				+ '<span v-for="strategy in strategyList">'
				+ '<strategy-ticker :strategy="strategy"></strategy-ticker>'
				+ '</span>'
				+ '<h1>Status</h1>'
				+ '<metric v-if="this.managerInfo" :text="\'Memory Usage\'" :value="memoryVirtualMemoryCurrent[0]" :unit="memoryVirtualMemoryCurrent[1]"></metric>'
				+ '<h3>Threads</h3>'
				+ '<threads :threadList="threadList"></threads>'
				+ '<h1>Traces</h1>'
				+ '<traces :traceList="traceList"></traces>'
				+ '</div>',
		props: ["exchangeList", "strategyList", "traceList", "threadList", "managerInfo"],
		computed: {
			memoryVirtualMemoryCurrent() {
				const size = this.managerInfo["memoryVirtualMemoryCurrent"];
				const unitList = ['B', 'kB', 'MB', 'GB', 'TB'];
				var i = Math.floor(Math.log(size) / Math.log(1024));
				i = Math.min(unitList.length - 1, i);
				const mem = (size / Math.pow(1024, i)).toFixed(2) * 1;
				return [mem, unitList[i]];
			}
		}
	};

	const viewChart = {
		template: '<div>'
				+ '<h1>Chart</h1>'
				+ '<div>'
				+ '<chart :data-list="dataList"></chart>'
				+ '<div></div>'
				+ '</div>'
				+ '</div>',
		props: ["data"],
		computed: {
			exchangeList() {
				return this.data.exchangeList;
			},
			dataList() {
				var list = [];
				for (var i in this.exchangeList) {
					list.push({
						title: "BTC/USD (" + this.exchangeList[i].getName() + ")",
						type: 'rate',
						url: "api/v1/exchange/" + i + "/rates/BTC/USD",
						timeseries: new TimeSeries({
							getTimestamp: function(entry) {
								return entry.t;
							},
							getData: function(entry) {
								return entry.r;
							}
						})
					});
					list.push({
						title: "USD/BTC (" + this.exchangeList[i].getName() + ")",
						type: 'rate',
						url: "api/v1/exchange/" + i + "/rates/USD/BTC",
						timeseries: new TimeSeries({
							getTimestamp: function(entry) {
								return entry.t;
							},
							getData: function(entry) {
								return 1. / entry.r;
							}
						})
					});
				}

				console.log(this.exchangeList);

				return list;
			}
		}
	};

	const viewExchange = {
		template: '<div v-if="isValid">'
				+ '<h1>{{ exchange.getName() }}</h1>'
				+ '<metric v-if="exchange.hasBalance()" :text="\'Estimate\'" :value="balanceEstimate" :unit="balanceEstimateUnit"></metric>'
				+ '<metric v-if="exchange.hasBalance()" :text="\'Profit\'" :value="balanceProfit" :unit="\'%\'"></metric>'
				+ '<metric :text="\'Trading Pairs\'" :value="nbTradingPairs"></metric>'
				+ '<h2>Configuration</h2>'
				+ '<properties :propertiesList="exchange.configuration"></properties>'
				+ '<div v-if="exchange.hasBalance()">'
				+ '<h2>Balance</h2>'
				+ '<balance :balance="exchange.getBalance()" :initial-balance="exchange.initialBalance" :key="this.$route.path"></balance>'
				+ '<h2>Orders</h2>'
				+ '<exchange-orders :exchange="exchange" :traceList="traceList"></exchange-orders>'
				+ '</div>'
				+ '<h2>Pairs</h2>'
				+ '<exchange-pairs :exchange="exchange"></exchange-pairs>'
				+ '</div>'
				+ '<div v-else>Exchange {{ id }} is invalid</div>',
		props: ["data", "id"],
		computed: {
			isValid() {
				return (typeof this.data.exchangeList[this.id] === "object");
			},
			exchange() {
				return this.data.exchangeList[this.id];
			},
			traceList() {
				return this.data.traceList;
			},
			balanceEstimate() {
				return this.exchange.getBalance().getEstimate().toFixed(1);
			},
			balanceEstimateUnit() {
				return this.exchange.getBalance().getEstimateCurrency();
			},
			balanceProfit() {
				return ((this.exchange.getBalance().getProfit() > 0) ? "+" : "")
						+ (this.exchange.getBalance().getProfit() * 100).toFixed(2);
			},
			nbTradingPairs() {
				return this.exchange.transactions.length;
			}
		}
	};

	const viewStrategy = {
		template: '<div v-if="isValid">'
				+ '<h1>{{ strategy.getName() }}</h1>'
				+ '<metric :text="\'Processing Time\'" :value="strategy.getProcessTimePercent().toFixed(1)" :unit="\'%\'"></metric>'
				+ '<h2>Configuration</h2>'
				+ '<properties :propertiesList="strategy.configuration"></properties>'
				+ '<div v-for="(profit, index) in strategy.getProfit()">'
					+ '<h2>Profit on {{ getExchangeName(index) }}</h2>'
					+ '<metric :text="\'Profit\'" :value="getProfit(profit)" :unit="profit.balance.getEstimateCurrency()"></metric>'
					+ '<metric :text="\'Success\'" :value="profit.nbSuccess" :unit="\'Operation(s)\'"></metric>'
					+ '<metric :text="\'Timeout\'" :value="profit.nbFailedTimeout" :unit="\'Operation(s)\'"></metric>'
					+ '<metric :text="\'Failed\'" :value="profit.nbFailedPlaceOrder" :unit="\'Operation(s)\'"></metric>'
					+ '<balance type="profit" :balance="profit.balance" :key="id"></balance>'
				+ '</div>'
				+ '<h2>Operation History</h2>'
				+ '<operations :operationList="strategy.operationList"></operations>'
				+ '<h2 v-if="hasData">Data</h2>'
				+ '<properties v-if="hasData" :propertiesList="strategy.data"></properties>'
				+ '</div>'
				+ '<div v-else>Strategy {{ id }} is invalid</div>',
		props: ["data", "id"],
		methods: {
			getExchangeName(index) {
				return (this.strategy.configuration["exchangeList"] && this.strategy.configuration["exchangeList"][index]) ?
						this.strategy.configuration["exchangeList"][index] : "Exchange " + index;
			},
			getProfit(profit) {
				return ((profit.balance.getEstimate() > 0) ? "+" : "") + profit.balance.getEstimate().toFixed(1);
			}
		},
		computed: {
			isValid() {
				return (typeof this.data.strategyList[this.id] === "object");
			},
			hasData() {
				for (var i in this.strategy.data) {
					return true;
				}
				return false;
			},
			strategy() {
				return this.data.strategyList[this.id];
			}
		}
	};

	const router = new VueRouter({
		routes: [
			{path: '/', component: viewDashboard, props: this.manager.data},
			{path: '/chart', component: viewChart, props: () => ({
				data: this.manager.data
			})},
			{path: '/exchange/:id', component: viewExchange, props: (route) => ({
				id: route.params.id,
				data: this.manager.data
			})},
			{path: '/strategy/:id', component: viewStrategy, props: (route) => ({
				id: route.params.id,
				data: this.manager.data
			})}
		]
	});

	return router;
};
