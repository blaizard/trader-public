Vue.component('exchange-orders', {
	template:
		'<div class="exchange-orders">'
			+ '<table class="records">'
				+ '<thead>'
					+ '<tr>'
						+ '<th class="type"></th>'
						+ '<th class="time">Time</th>'
						+ '<th class="id">Id</th>'
						+ '<th class="order-type">Type</th>'
						+ '<th class="strategy">Strategy</th>'
						+ '<th class="context">Context</th>'
						+ '<th class="pair">Pair</th>'
						+ '<th class="rate">Rate</th>'
						+ '<th class="amount">Amount</th>'
						+ '<th class="notification"></th>'
						+ '<th class="message"></th>'
					+ '</tr>'
				+ '</thead>'
				+ '<tbody>'
					+ '<tr class="order" v-for="order in orderList" v-on:click="filterByContext(order.context)">'
						+ '<td class="type"><i class="fa fa-cog fa-spin fa-fw"></i> Active</td>'
						+ '<td class="time">{{ (new Date(order.creationTime)).toLocaleString() }}</td>'
						+ '<td class="id">{{ order.id }}</td>'
						+ '<td class="order-type">{{ order.orderType }}</td>'
						+ '<td class="strategy">{{ order.strategy }}</td>'
						+ '<td class="context">{{ order.context }}</td>'
						+ '<td class="pair">{{ order.initialCurrency + "/" + order.finalCurrency }}</td>'
						+ '<td class="rate">{{ order.rate }} ({{ getCurrentRate(order) }})</td>'
						+ '<td class="amount">{{ order.amount }}</td>'
						+ '<td class="notification"></td>'
						+ '<td class="message">timeout={{ order.timeout }}s</td>'
					+ '</tr>'
					+ '<template v-for="(record, index) in recordList">'
						+ '<tr class="record-item" v-bind:class="{ showNotification: showNotification == index }">'
							+ '<td class="type" v-html="htmlOrderType(record)"></td>'
							+ '<td class="time">{{ (new Date(record.timestamp)).toLocaleString() }}</td>'
							+ '<td class="id">{{ record.id }}</td>'
							+ '<td class="order-type">{{ record.orderType }}</td>'
							+ '<td class="strategy">{{ record.strategy }}</td>'
							+ '<td class="context">{{ record.context }}</td>'
							+ '<td class="pair">{{ record.initialCurrency + "/" + record.finalCurrency }}</td>'
							+ '<td class="rate">{{ record.rate }}</td>'
							+ '<td class="amount">{{ record.amount }}</td>'
							+ '<td class="notification">'
								+ '<div class="item" v-on:click="toggleNotification(index)" v-if="traceListForId(record).length">'
									+ '<i class="fa fa-exclamation-triangle" aria-hidden="true"></i><span class="nb">{{ traceListForId(record).length }}</span>'
								+ '</div>'
							+ '</td>'
							+ '<td class="message">{{ record.message }}</td>'
						+ '</tr>'
						+ '<tr class="record-notification">'
							+ '<td colspan="11">'
								+ '<div v-for="trace in traceListForId(record)">{{ trace.message }}</div>'
							+ '</td>'
						+ '</tr>'
					+ '</template>'
				+ '</tbody>'
			+ '</table>'
		+ '</div>',
	props: ['exchange', 'traceList'],
	data() {
		return {
			showNotification: -1,
			filterContext: null
		};
	},
	mounted() {
		this.exchange.interval.update("updateOrders", 5000);
		this.exchange.interval.update("updateOrderRecords", 5000);
	},
	methods: {
		traceListForId: function (record) {
			var idRegex = record.id.replace(/([.*+?^=!:${}()|\[\]\/\\])/g, "\\$1");
			var regex = new RegExp(idRegex + "\\b", "i");
			return this.traceList.filter(function(trace) {
				return regex.test(trace.message);
			})
		},
		htmlOrderType(record) {
			const orderType = {
				"cancel": "<i class=\"fa fa-times\" aria-hidden=\"true\"></i> Cancel",
				"failed": "<i class=\"fa fa-times\" aria-hidden=\"true\"></i> Failed",
				"timeout": "<i class=\"fa fa-hourglass\" aria-hidden=\"true\"></i> Timeout",
				"partial": "<i class=\"fa fa-star-half-o\" aria-hidden=\"true\"></i> Partial",
				"proceed": "<i class=\"fa fa-star\" aria-hidden=\"true\"></i> Proceed",
				"place": "<i class=\"fa fa-star-o\" aria-hidden=\"true\"></i> Place"
			};
			return orderType[record.type];
		},
		toggleNotification: function(index) {
			this.showNotification = (this.showNotification == index) ? -1 : index;
		},
		filterByContext: function(context) {
			this.filterContext = (this.filterContext == context) ? null : context;
			console.log(this.filterContext);
		},
		getCurrentRate(order) {
			for (var i in this.exchange.rates) {
				if (this.exchange.rates[i].initialCurrency == order.initialCurrency
						&& this.exchange.rates[i].finalCurrency == order.finalCurrency) {
					return ((this.exchange.rates[i].rate - order.rate) / order.rate * 100).toFixed(1) + "%";
				}
			}
			return "-";
		}
	},
	computed: {
		orderList() {
			return this.exchange.orders;
		},
		recordList() {
			var list = [];
			this.exchange.orderRecords.each((timestamp, item) => {
				if (this.filterContext && item.context != this.filterContext) {
					return;
				}
				list.unshift(item);
			});
			return list;
		}
	}
});
