Vue.component('properties', {
	template:
		'<div class="properties">'
			+ '<div class="item" v-for="(value, name) in propertiesList">'
				+ '<span class="name">{{ name }}:</span>'
				+ '<span v-if="value instanceof Array" class="array">'
					+ '<span v-for="v in value" class="value">{{ v }}</span>'
				+ '</span>'
				+ '<span v-else-if="value instanceof Object" class="object">'
					+ '<span v-for="(v, key) in value" class="value">{{ key }}: {{ v }}</span>'
				+ '</span>'
				+ '<span v-else class="value">{{ value }}</span>'
			+ '</div>'
		+ '</div>',
	props: ['propertiesList'],
	data() {
		return {};
	},
	computed: {
	}
});
