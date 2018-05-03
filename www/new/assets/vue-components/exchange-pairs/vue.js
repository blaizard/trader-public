Vue.component('exchange-pairs', {
	template: '<div class="exchange-pairs">'
		+ '<table>'
		+ '<thead><tr>'
		+ '<th>Pair</th><th>Decimal (Order)</th><th>Boundaries</th><th>Fee</th><th>Rate</th>'
		+ '</tr></thead>'
		+ '<tbody>'
		+ '<tr v-for="transaction in transactions">'
		+ '<td>{{ transaction.initialCurrency }}/{{ transaction.finalCurrency }}</td>'
		+ '<td>{{ transaction.decimal }} ({{ transaction.decimalOrder }})</td>'
		+ '<td>{{ transaction.boundaries }}</td>'
		+ '<td>{{ transaction.fee }}</td>'
		+ '<td><span v-bind:class="rateList[getKey(transaction)][1]">{{ rateList[getKey(transaction)][0] }}</span></td>'
		+ '</tr>'
		+ '</tbody>'
		+ '</table>'
		+ '</div>',
	props: ['exchange'],
	data() {
		return {
			prevRateList: {}
		};
	},
	mounted() {
		this.exchange.interval.update("updateRates", 2000);
	},
	methods: {
		getKey: function(object) {
			return object.initialCurrency + "/" + object.finalCurrency;
		}
	},
	computed: {
		rateList() {
			for (var i in this.exchange.rates) {
				var key = this.getKey(this.exchange.rates[i]);
				var rate = this.exchange.rates[i].rate;
				var prevRate = (this.prevRateList.hasOwnProperty(key) && this.prevRateList[key][0] != rate) ?
						((this.prevRateList[key][0] < rate) ? "positive" : "negative") : "";
				this.prevRateList[key] = [rate, prevRate];
			}
			return this.prevRateList;
		},
		transactions() {
			return this.exchange.transactions;
		},
		currencyList() {
			var currencyList = [];
			for (var i in this.transactions) {
				if (currencyList.indexOf(this.transactions[i].initialCurrency) == -1) {
					currencyList.push(this.transactions[i].initialCurrency);
				}
				if (currencyList.indexOf(this.transactions[i].finalCurrency) == -1) {
					currencyList.push(this.transactions[i].finalCurrency);
				}
			}
			return currencyList.sort();
		}
	}
});
