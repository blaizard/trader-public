function VueComponent(id) {
	return Vue.options.components[id];
}

// Define dependencies
irRequire.map = {
	"irAjax": "new/lib/irajax/irajax.js",
	"Manager": ["new/assets/trader/manager.js", "Interval"],
	"Exchange": ["new/assets/trader/exchange.js", "Interval"],
	"Strategy": ["new/assets/trader/strategy.js", "Interval"],
	"Balance": "new/assets/trader/balance.js",
	"Interval": "new/assets/trader/interval.js",
	"TimeSeries": "new/assets/trader/time-series.js",
	"moment" : ["new/lib/moment/moment.min.js"],
	"Chart" : ["moment", "new/lib/chartjs/chart.min.js"],
	"Vue": "new/lib/vue/vue.js",
	"VueRouter": ["Vue", "new/lib/vue/vue-router.js"],
	"VueComponent('exchange-ticker')": ["Vue", "new/assets/vue-components/exchange-ticker/vue.js", "new/assets/vue-components/exchange-ticker/style.css"],
	"VueComponent('exchange-pairs')": ["Vue", "new/assets/vue-components/exchange-pairs/vue.js", "new/assets/vue-components/exchange-pairs/style.css"],
	"VueComponent('exchange-orders')": ["Vue", "new/assets/vue-components/exchange-orders/vue.js", "new/assets/vue-components/exchange-orders/style.css"],
	"VueComponent('strategy-ticker')": ["Vue", "new/assets/vue-components/strategy-ticker/vue.js", "new/assets/vue-components/strategy-ticker/style.css"],
	"VueComponent('balance')": ["Vue", "new/assets/vue-components/balance/vue.js", "new/assets/vue-components/balance/style.css"],
	"VueComponent('metric')": ["Vue", "new/assets/vue-components/metric/vue.js", "new/assets/vue-components/metric/style.css"],
	"VueComponent('chart')": ["Vue", "new/assets/vue-components/chart/style.css", "Chart", "new/assets/vue-components/chart/vue.js"],
	"VueComponent('properties')": ["Vue", "new/assets/vue-components/properties/style.css", "new/assets/vue-components/properties/vue.js"],
	"VueComponent('traces')": ["Vue", "new/assets/vue-components/traces/style.css", "new/assets/vue-components/traces/vue.js"],
	"VueComponent('operations')": ["Vue", "new/assets/vue-components/operations/style.css", "new/assets/vue-components/operations/vue.js"],
	"VueComponent('threads')": ["Vue", "new/assets/vue-components/threads/style.css", "new/assets/vue-components/threads/vue.js"],
	"App": "new/assets/page/app.js",
	"jQuery": "new/lib/jquery/jquery-3.2.1.min.js"
};
