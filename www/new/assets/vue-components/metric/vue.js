Vue.component('metric', {
	template: '<div class="metric">'
		+ '<div class="text">{{ text }}</div>'
		+ '<div class="value">{{ value }}</div>'
		+ '<div class="unit">{{ unit }}</div>'
		+ '</div>',
	props: ['text', 'value', 'unit'],
	data() {
		return {};
	},
	computed: {
	}
});
