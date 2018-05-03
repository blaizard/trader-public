Vue.component('exchange-ticker', {
	template: '<div class="exchange-ticker" @click.stop="goToPage" v-bind:class="exchange.getStatus()">'
		+ '<div class="status">{{ exchange.getStatus() }}</div>'
		+ '<div class="name"><i class="fa fa-server" aria-hidden="true"></i> {{ exchange.getName() }}</div>'
		+ '<div class="balance" v-if="exchange.hasBalance()"><span :class="((profitPercent > 0) ? \'positive\' : \'negative\')">{{ balance }}</span> <span class="profit">({{ ((profitPercent > 0) ? "+" : "") + profitPercent }}%)</span></div>'
		+ '<div class="time">{{ serverTime }}</div>'
		+ '<div class="message">{{ message }}</div>'
		+ '</div>',
	props: ['exchange'],
	data() {
		return {
			time: 0
		};
	},
	mounted() {
		this.interval = setInterval(() => {
			this.time = this.exchange.getTimestamp();
		}, 1000);
	},
	beforeDestroy() {
		clearInterval(this.interval);
	},
	methods: {
		timestampToDate(timestamp) {
			return (new Date(timestamp)).toLocaleString();
		},
		timestampToTime(timestamp) {
			return (new Date(timestamp)).toLocaleTimeString();
		},
		goToPage() {
			window.location.href = "new/index.html#/exchange/" + this.exchange.getId();
		}
	},
	computed: {
		balance() {
			return this.exchange.getBalance().getEstimateString();
		},
		profitPercent() {
			var profit = this.exchange.getBalance().getProfit();
			return (profit * 100).toFixed(2) / 1.;
		},
		message() {
			if (this.exchange.isConnected()) {
				return "Connected since: " + this.timestampToDate(this.exchange.getTimestampConnected());
			}
			else if (this.exchange.isDisconnected())
			{
				if (this.exchange.getTimestampConnected()) {
					return "Disconnected (last connection: " + this.timestampToDate(this.exchange.getTimestampConnected()) + ")";
				}
				return "Disconnected";
			}
			return this.exchange.getStatus() + "...";
		},
		serverTime() {
			return "Server Time: " + ((this.time) ? this.timestampToTime(this.time) : "-");
		}
	}
});
