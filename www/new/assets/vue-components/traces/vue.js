Vue.component('traces', {
	template:
		'<div class="traces">'
			+ '<table>'
				+ '<thead>'
					+ '<tr>'
						+ '<th class="timestamp">Timestamp</th>'
						+ '<th class="level">Level</th>'
						+ '<th class="topic">Topic</th>'
						+ '<th class="message">Message</th>'
					+ '</tr>'
				+ '</thead>'
				+ '<tbody>'
					+ '<tr v-for="trace in traceList">'
						+ '<td class="timestamp">{{ (new Date(trace.timestamp)).toLocaleString() }}</td>'
						+ '<td class="level">{{ trace.level }}</td>'
						+ '<td class="topic">{{ trace.topic }}</td>'
						+ '<td class="message">{{ trace.message }}</td>'
					+ '</tr>'
				+ '</tbody>'
			+ '</table>'
		+ '</div>',
	props: ['traceList'],
	data() {
		return {};
	},
	computed: {
	}
});
