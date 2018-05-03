Vue.component('threads', {
	template:
		'<div class="threads">'
			+ '<div v-for="thread in sortedThreadList" class="thread">'
				+ '<div v-bind:class="{ active: thread.isActive, status: true }"></div>{{ thread.name }}'
			+ '</div>'
		+ '</div>',
	props: ['threadList'],
	data() {
		return {};
	},
	computed: {
		sortedThreadList() {
			return this.threadList.sort((a, b) => {
				return (a.name > b.name) ? 1 : ((b.name > a.name) ? -1 : 0);
			});
		}
	}
});
