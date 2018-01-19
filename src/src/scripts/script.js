var data = {
	control: {},
	dataMap: {
		mode: [{
				value: 'AUTO',
				icon: 'radio-list-item icon auto'
			}, {
				value: 'DRY',
				icon: 'radio-list-item icon dry'
			}, {
				value: 'COOL',
				icon: 'radio-list-item icon cool'
			}, {
				value: 'HEAT',
				icon: 'radio-list-item icon heat'
			}, {
				value: 'FAN',
				icon: 'radio-list-item icon fan'
			}
		],
		fan: [{
				value: 'AUTO',
				icon: 'radio-list-item icon auto'
			}, {
				value: 'QUIET',
				icon: 'radio-list-item icon quiet'
			}, {
				value: '1',
				icon: 'radio-list-item icon oneoffour'
			}, {
				value: '2',
				icon: 'radio-list-item icon twooffour'
			}, {
				value: '3',
				icon: 'radio-list-item icon threeoffour'
			}, {
				value: '4',
				icon: 'radio-list-item icon fouroffour'
			}
		],
		vane: [{
				value: 'AUTO',
				icon: 'radio-list-item icon auto'
			}, {
				value: '1',
				icon: 'radio-list-item icon oneoffive'
			}, {
				value: '2',
				icon: 'radio-list-item icon twooffive'
			}, {
				value: '3',
				icon: 'radio-list-item icon threeoffive'
			}, {
				value: '4',
				icon: 'radio-list-item icon fouroffive'
			}, {
				value: '5',
				icon: 'radio-list-item icon fiveoffive'
			}, {
				value: 'SWING',
				icon: 'radio-list-item icon swing'
			}
		],
		wideVane: [{
				value: '<<',
				icon: 'radio-list-item icon leftleft'
			}, {
				value: '<',
				icon: 'radio-list-item icon left'
			}, {
				value: '|',
				icon: 'radio-list-item icon down'
			}, {
				value: '>',
				icon: 'radio-list-item icon right'
			}, {
				value: '>>',
				icon: 'radio-list-item icon rightright'
			}, {
				value: '<>',
				icon: 'radio-list-item icon leftright'
			}, {
				value: 'SWING',
				icon: 'radio-list-item icon swing'
			}
		]
	},
	wifi: {},
	mqtt: {},
	log: [{
			level: 'info',
			message: 'page loaded'
		}
	],
	controlVisable: true,
	logVisable: false,
	wifiVisable: false,
	mqttVisable: false
}
rivets.formatters.power = {
	read: function (value) {
		return value == "ON";
	},
	publish: function (value) {
		return value ? "ON" : "OFF"
	}
}
setAllInvisable = function () {
	data.controlVisable = false;
	data.logVisable = false;
	data.wifiVisable = false;
	data.mqttVisable = false;
}
setControlVisable = function () {
	setAllInvisable();
	data.controlVisable = true;
}
setLogVisable = function () {
	setAllInvisable();
	data.logVisable = true;
}
setWifiVisable = function () {
	setAllInvisable();
	data.wifiVisable = true;
}
setMqttVisable = function () {
	setAllInvisable();
	data.mqttVisable = true;
}
var websock;
var es;
var timeout;
rivets.formatters.notify = {
	read: function (value) {
		return value;
	},
	publish: function (value) {
		clearTimeout(timeout);
		timeout = setTimeout(function () {
				websock.send(JSON.stringify(data.control));
				console.log(JSON.stringify(data.control));
			}, 3000);
		return value;
	}
}
appstart = function () {
	rivets.bind(document.querySelector('#main'), {
		data: data
	});
	websock = new WebSocket('ws://' + window.location.hostname + '/ws', ['arduino']);
	websock.onopen = function (evt) {
		console.log('websock open');
	};
	websock.onclose = function (evt) {
		console.log('websock close');
	};
	websock.onerror = function (evt) {
		console.log(evt);
	};
	websock.onmessage = function (evt) {
		console.log(evt);
		var json = JSON.parse(evt.data);
		if (json.control) {
			data.control = json.control;
		}
		if (json.log) {
			for (var i = 0; i < json.log.length; i++) {
				data.log.push(json.log[i]);
			}
		}
	};
}