Vue.component('strategy-ticker', {
	template: '<div class="strategy-ticker" @click.stop="goToPage">'
		+ '<div class="name"><i class="fa fa-cogs" aria-hidden="true"></i> {{ strategy.getName() }}</div>'
		+ '<div class="profit">Profit: {{ profitEstimate }}</div>'
		+ '<div class="nb-operations">Nb. Operations: {{ nbSuccess + nbFailedTimeout + nbFailedPlaceOrder }}</div>'
		+ '<div class="performance">Processing Time: <progress max="100" v-bind:value="strategy.getProcessTimePercent()"></progress></div>'
		+ '</div>',
	props: ['strategy'],
	data() {
		return {};
	},
	methods: {
		goToPage() {
			window.location.href = "new/index.html#/strategy/" + this.strategy.getId();
		}
	},
	computed: {
		profitEstimate() {
			// Combine the data
			var profitEstimate = {};
			for (var i in this.strategy.getProfit()) {
				var balance = this.strategy.profit[i].balance;
				var currency = balance.getEstimateCurrency();
				var amount = balance.getEstimate();
				if (!profitEstimate[currency]) {
					profitEstimate[currency] = 0;
				}
				profitEstimate[currency] += amount;
			}
			// Build the string
			var str = "";
			for (var currency in profitEstimate) {
				str += ((str) ? ", " : "");
				str += ((profitEstimate[currency] > 0) ? "+" : "") + profitEstimate[currency].toFixed(2) + " " + currency;
			}

			return str;
		},
		nbSuccess() {
			var counter = 0;
			for (var i in this.strategy.getProfit()) {
				counter += this.strategy.profit[i].nbSuccess;
			}
			return counter;
		},
		nbFailedTimeout() {
			var counter = 0;
			for (var i in this.strategy.getProfit()) {
				counter += this.strategy.profit[i].nbFailedTimeout;
			}
			return counter;
		},
		nbFailedPlaceOrder() {
			var counter = 0;
			for (var i in this.strategy.getProfit()) {
				counter += this.strategy.profit[i].nbFailedPlaceOrder;
			}
			return counter;
		}
	}
});
