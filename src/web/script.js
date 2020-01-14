const host = "http://192.168.1.1/";
const stateColors = ['#173F5F', '#20639B', '#3CAEA3'];
const TimeoutError = "TimeoutError";
const UnsuccessfulError = "UnsuccessfulError";
let online = false;
let network;
let view;
let status;

//=============================================================================
function withTimeout(msecs, promise) {
	const timeout = new Promise((resolve, reject) => {
		setTimeout(() => {
			const e = new Error(`request takes more than ${msecs}`);
			e.name = TimeoutError;
			reject(e);
		}, msecs);
	});
	return Promise.race([timeout, promise]);
}

const keys = {9: 1, 32: 1, 33: 1, 34: 1, 35: 1, 36: 1, 37: 1, 38: 1, 39: 1, 40: 1,};

function preventDefault(e) {
	e = e || window.event;
	if (e.preventDefault) {
		e.preventDefault();
	}
	e.returnValue = false;
}

function preventDefaultForScrollKeys(e) {
	return false;
}

function disableScroll() {
	if (window.addEventListener) // older FF
		window.addEventListener('DOMMouseScroll', preventDefault, false);
	document.addEventListener('wheel', preventDefault, {passive: false}); // Disable scrolling in Chrome
	window.onwheel = preventDefault; // modern standard
	window.onmousewheel = document.onmousewheel = preventDefault; // older browsers, IE
	window.ontouchmove = preventDefault; // mobile
	document.onkeydown = preventDefaultForScrollKeys;
}

function enableScroll() {
	if (window.removeEventListener) window.removeEventListener('DOMMouseScroll', preventDefault, false);
	document.removeEventListener('wheel', preventDefault, {passive: false}); // Enable scrolling in Chrome
	window.onmousewheel = document.onmousewheel = null;
	window.onwheel = null;
	window.ontouchmove = null;
	document.onkeydown = null;
}

function showLoadingScreen() {
	view.loadingScreen.classList.remove("hide");
	disableScroll();
}

function hideLoadingScreen() {
	view.loadingScreen.classList.add("hide");
	enableScroll();
}

class View {
	constructor() {
		this.stateTimeline 	= select("#state-timeline");
		this.valveOverview 	= select("#valve-overview");
		this.startButton 	= select("#start-button");
		this.stopButton 	= select("#stop-button");
		this.connection 	= select("#connection");
		this.connectionIndicator = select("#connection-indicator");
		this.loadingScreen = select("#loading-screen");

		this.stateConfigs = {
			flush: Array.from(selectAll(".state-config[data-name='flush'] .state-property")).reduce((acc, cur) => {
				acc[cur.dataset.name]  	= cur.querySelector(".input");
				return acc;
			}, {}),
			sample: Array.from(selectAll(".state-config[data-name='sample'] .state-property")).reduce((acc, cur) => {
				acc[cur.dataset.name]  	= cur.querySelector(".input");
				return acc;
			}, {})
		};
	}

	createElements() {
		// Crate and initialize state node header
		const states = ["flush", "sample"];
		this.stateNodes = states.map((stateName, index) => {
			const node = document.createElement("button");
			node.classList.add("state-node");
			node.style.background = "white";
			node.style.color = stateColors[index % stateColors.length];
			node.textContent = stateName.toUpperCase();
			node.dataset.name = stateName;
			this.stateTimeline.appendChild(node);
			return node;
		});


		// Create and initialize valve overview header
		this.valves = [...Array(24).keys()].map(i => {
			const valve = document.createElement("button");
			valve.textContent = `${i}`;
			valve.disabled = true;
			valve.onclick = () => {
				if (valve.classList.contains("selected")) {
					valve.classList.remove("selected")
				} else {
					this.deselectValves();
					valve.classList.add("selected");
				}
			};

			return valve;
		});

		// Append to the overview in reverse order for the first half of the valves
		for (let i = 0; i < this.valves.length; i++) {
			this.valveOverview.append(this.valves[i < 12 ? 11 - i : i]);
		}

		// Apply styling to state configs
		Array.from(selectAll(".state-config")).forEach(config => {
			const index = states.findIndex(name => config.dataset.name === name);
			config.getElementsByClassName("heading")[0].style.background = stateColors[index % stateColors.length];
		});
	}

	deselectValves() {
		this.valves.forEach(v => {v.classList.remove("selected");})
	}

	generateTask() {
		return {
			flushTime: parseInt(this.stateConfigs.flush.time.value),
			sampleTime: parseInt(this.stateConfigs.sample.time.value),
			valve: this.valves.findIndex(v => v.classList.contains("selected"))
		};
	}

	updateStatus(data) {
		const {stateName = "undefined", stateId = NaN, valveCurrent = NaN, valves = []} = data;
		valves.forEach((value, index) => {
			this.valves[index].classList.remove("current");
			this.valves[index].disabled = (value === 0);
		});

		if (valveCurrent !== -1) {
			this.valves[valveCurrent].classList.remove("selected");
			this.valves[valveCurrent].classList.add("current");
		}
	}
}

class Network {
	defaultTimeout = 5000;
	handleResponse(res) {
		return res.json().then(res => {
			const {error, success, content} = res;
			if (error) {
				const e = new Error(error);
				e.name = UnsuccessfulError;
				throw e;
			}

			return content;
		});
	}

	stop() {
		const promise = fetch(host + "stop", {method: "get"});
		return withTimeout(this.defaultTimeout, promise).then(res => this.handleResponse(res));
	}

	start(task) {
		const promise = fetch(host + "start", {method: "POST", body: JSON.stringify(task)});
		return withTimeout(this.defaultTimeout, promise).then(res => this.handleResponse(res));
	}
}

class Status {
	constructor() {
		//@formatter:off
		this.live			= false;
		this.requesting 	= false;
		this.interval 		= 1000;
	}	//@formatter:on

	fetchStatusUpdate() {
		if (this.requesting) {
			return;
		}

		withTimeout(5000, fetch(host + "status", {method: "GET"})).then(res => res.json()).then(res => {
			view.updateStatus(res);
			view.connection.textContent = "Online";
			view.connectionIndicator.classList.remove("offline");
			if (this.live) {
				this.timer = setTimeout(() => {this.fetchStatusUpdate();}, this.interval);
			}
		}).catch(error => {
			switch (error.name) {
			case TimeoutError:
				view.connection.textContent = "Server Timeout";
				view.connectionIndicator.classList.add("offline");
				console.log("Timeout Error: Trying again in 5 secs");
				break;
			default:
				online = false;
				view.connection.textContent = "Offline";
				view.connectionIndicator.classList.add("offline");
				console.log("Network Error: Trying again in 5 secs");
				break;
			}

			if (this.live) {
				clearTimeout(this.timer);
				this.timer = setTimeout(() => {this.fetchStatusUpdate();}, 5000);
			}
		}).finally(() => {
			this.requesting = false;
		});
	}

	beginLiveStatusUpdate(interval) {
		clearTimeout(this.timer);
		this.live = true;
		if (interval) {
			this.interval = interval;
		}
		this.fetchStatusUpdate();
	}

	stopLiveStatusUpdate() {
		clearTimeout(this.timer);
		this.live = false;
		this.requesting = false;
	}
}

function select(query) {
	return document.querySelector(query)
}

function selectAll(query) {
	return document.querySelectorAll(query);
}

function startOperation(task) {
	view.deselectValves();
	showLoadingScreen();
	network.start(task).finally(_ => {
		hideLoadingScreen();
	});
}

function stopOperation() {
	view.deselectValves();
	showLoadingScreen();
	network.stop().finally(_ => {
		hideLoadingScreen();
	});
}

window.onload = function() {
	status = new Status();
	network = new Network();

	view = new View();
	view.createElements();

	view.startButton.onclick = () => {
		const task = view.generateTask();
		if (task.valve === -1) {
			console.log("No valve selected");
			return;
		}

		if (isNaN(task.flushTime)) {
			// Highlight
			console.log("Missing flush time");
			return;
		}

		if (isNaN(task.sampleTime)) {
			// Highlight
			console.log("Missing Sample time");
			return;
		}

		startOperation(task);
	};

	// Stop operation
	view.stopButton.onclick = () => {
		stopOperation();
	};

	status.beginLiveStatusUpdate();
};