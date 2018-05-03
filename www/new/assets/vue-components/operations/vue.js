Vue.component('operations', {
	template:
		'<div class="operations">'
			+ '<table>'
				+ '<thead>'
					+ '<tr>'
						+ '<th class="status"></th>'
						+ '<th class="id">Id</th>'
						+ '<th class="timestamp">Timestamp</th>'
						+ '<th class="timestamp">Profit</th>'
						+ '<th class="timestamp">Currency</th>'
						+ '<th class="description">Description</th>'
					+ '</tr>'
				+ '</thead>'
				+ '<tbody>'
					+ '<tr v-for="operation in operationList">'
						+ '<td class="status" v-html="htmlStatus(operation)"></td>'
						+ '<td class="id">{{ operation.id }}</td>'
						+ '<td class="timestamp">{{ (new Date(operation.timestamp)).toLocaleString() }}</td>'
						+ '<td class="profit">{{ operation.profit }}</td>'
						+ '<td class="currency">{{ operation.currency }}</td>'
						+ '<td class="description">{{ operation.description }}</td>'
					+ '</tr>'
				+ '</tbody>'
			+ '</table>'
		+ '</div>',
	props: ['operationList'],
	data() {
		return {};
	},
	computed: {
	},
	methods: {
		htmlStatus(operation) {
			const operationStatus = {
				"failed": "<i class=\"fa fa-times\" aria-hidden=\"true\"></i> Failed",
				"timeout": "<i class=\"fa fa-hourglass\" aria-hidden=\"true\"></i> Timeout",
				"success": "<i class=\"fa fa-star\" aria-hidden=\"true\"></i> Success"
			};
			return operationStatus[operation.status];
		}
	}
});
