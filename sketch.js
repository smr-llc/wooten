
let buf = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

let connections = [];

let isRecording = false;

function setup() {
	//Create a canvas of dimensions given by current browser window
	createCanvas(windowWidth, windowHeight);

	//text font
	textFont('Courier New');

	monitorSelfToggle = createCheckbox("Monitor", false);
	monitorSelfSlider = createSlider(1, 1000, 900);
  	monitorSelfToggle.changed(checkEnabled);

	sessionInput = createInput('');
	joinSessionBtn = createButton('Join Session');
	joinSessionBtn.mousePressed(joinSession);
	createSessionBtn = createButton('Create Session');
	createSessionBtn.mousePressed(createSession);
	leaveSessionBtn = createButton('Leave Session');
	leaveSessionBtn.mousePressed(leaveSession);

	recordBtn = createButton('Record');
	recordBtn.mousePressed(record);

	formatDOMElements();
	checkEnabled();
}

function toDbScale(val) {
	let db = Math.log10(val) * 20
	if (db < -40) {
		return 0.05 - (0.05 * (Math.max(db + 40, -30) / -30))
	}
	else if (db < -25) {
		return 0.05 + 0.20 - (0.20 * (Math.max(db + 25, -15) / -15))
	}
	else {
		return 0.25 + 0.80 * (1 - (db / -25))
	}
}

function drawLevelBar(x, y, maxW, maxH, avg, peak, heldPeak) {
	avg = toDbScale(avg)
	peak = toDbScale(peak)
	heldPeak = toDbScale(heldPeak)
	noStroke();
	fill(20);
	rect(x, y, maxW, maxH);
	fill(25, 150, 25);
	rect(x, y, maxW * peak, maxH);
	fill(50, 200, 50);
	rect(x, y, maxW * avg, maxH);
	fill(85, 215, 85);
	if (heldPeak > 0.6) {
		fill(85 + ((heldPeak - 0.6) * 300), 215 - ((heldPeak - 0.6) * 300), 85)
	}
	rect(x + maxW * .99 * heldPeak, y, maxW*0.01, maxH);
}

function createConnection() {

	let rxLevelSlider = createSlider(1, 1000, 900);
	let rxQueueSlider = createSlider(1, 32, 8);

	return {
		x: 0,
		y: 0,
		w: 200,
		h: 100,
		levelSlider: rxLevelSlider,
		queueSlider: rxQueueSlider
	}
}

function adjustConnections() {
	let rowH = 0.025 * windowHeight;
	let colW = 0.05 * windowWidth

	for(let i = 0; i < connections.length; i++) {
		conn = connections[i]
		conn.x = 0
		conn.y = rowH*9 + (i * rowH * 4.5)
		conn.w = colW*20
		conn.h = rowH*4

		let rowC = 0.25 * conn.h;
		let colC = 0.05 * conn.w
		elRect(conn.levelSlider, colC*12, conn.y + rowC, colC*7, rowC);
		elRect(conn.queueSlider, colC*4, conn.y + rowC*2, colC*15, rowC);
	}
}

function drawConnection(c, readBuf, writeBuf) {
	let rowH = 0.25 * c.h;
	let colW = 0.05 * c.w;

	let name = Bela.data.buffers[readBuf];
	let lvlBuf = Bela.data.buffers[readBuf+1];

	if (!name) {
		return -1;
	}

	// draw background panel
    fill(40);
    noStroke();
    rect(c.x, c.y, c.w, c.h);

	// draw level bar
	if (lvlBuf) {
		drawLevelBar(c.x + colW*4, c.y+rowH, colW*7.0, rowH, lvlBuf[0], lvlBuf[1], lvlBuf[2]);
	}
	fill(200);
	textSize(fontSize(colW, rowH));
	textAlign(RIGHT);
	text(name.join("") + " Level:", c.x, c.y+rowH, colW*3.5, rowH);
	text("Speed <-> Quality", c.x, c.y+rowH*2, colW*3.5, rowH);

	let sendBuf = [c.queueSlider.value(), c.levelSlider.value()];
	Bela.data.sendBuffer(writeBuf, 'int', sendBuf);

	return 2; // number of read buffers read
}

function draw() {

    background(60);

	let inL = Bela.data.buffers[1];
	let inR = Bela.data.buffers[2];
	
	let rowH = 0.025 * windowHeight;
	let colW = 0.05 * windowWidth

	if (inL) {
		drawLevelBar(colW*4.5, rowH*1, colW*14, rowH*0.5, inL[0], inL[1], inL[2]);
		drawLevelBar(colW*4.5, rowH*1.5, colW*14, rowH*0.5, inR[0], inR[1], inR[2]);
	}

	fill(200);
	textSize(fontSize(colW, rowH));
	textAlign(RIGHT);
	text("Send Level:", 0, rowH*1, colW*4, rowH);

	buf[0] = monitorSelfToggle.checked() ? 1 : 0;
	buf[1] = monitorSelfSlider.value();
    Bela.data.sendBuffer(0, 'int', buf);


	let activeSession = Bela.data.buffers[3];
	if (activeSession == 1) {
		let sessionName = Bela.data.buffers[4];
		sessionInput.attribute('disabled', 'true');
		sessionInput.value(sessionName.join(""));
		joinSessionBtn.attribute('disabled', 'true');
		createSessionBtn.attribute('disabled', 'true');
		leaveSessionBtn.removeAttribute('disabled');
	}
	else {
		sessionInput.removeAttribute('disabled');
		joinSessionBtn.removeAttribute('disabled');
		createSessionBtn.removeAttribute('disabled');
		leaveSessionBtn.attribute('disabled', 'true');
	}

	let rxCount = Bela.data.buffers[9];
	if (rxCount === undefined) {
		rxCount = 0;
	}
	while (connections.length > rxCount) {
		let c = connections.pop();
		c.levelSlider.remove();
		c.queueSlider.remove();
	}
	if (rxCount > 0) {
		let adjust = false
		for (let i = 0; i < rxCount; i++) {
			if ((i + 1) > connections.length) {
				connections.push(createConnection());
				adjust = true
			}
		}
		if (adjust) {
			adjustConnections()
		}
		let readBufOffset = 10;
		let writeBufOffset = 1;
		for (let i = 0; i < connections.length; i++) {
			let result = drawConnection(connections[i], readBufOffset, writeBufOffset);
			if (result < 1) {
				continue;
			}
			readBufOffset += result;
			writeBufOffset += 1;
		}
	}

}

function windowResized() {
	formatDOMElements()
	adjustConnections()
	resizeCanvas(windowWidth, windowHeight)
}

function formatDOMElements() {
	let rowH = 0.025 * windowHeight;
	let colW = 0.05 * windowWidth

	monitorSelfToggle.style('font',  pxFont(colW, rowH) + ' "Courier New"');
	monitorSelfToggle.style('color', color(200));
	monitorSelfToggle.style('text-align', 'right');

	elRect(monitorSelfToggle, colW*0.5, rowH*3, colW*3.5, rowH);
	elRect(monitorSelfSlider, colW*4.5, rowH*3, colW*15, rowH);

	elRect(sessionInput, colW, rowH*5.5, colW*3, rowH*2);
	elRect(joinSessionBtn, colW*4.5, rowH*5.5, colW*3, rowH*2);
	elRect(createSessionBtn, colW*8, rowH*5.5, colW*3, rowH*2);
	elRect(leaveSessionBtn, colW*11.5, rowH*5.5, colW*3, rowH*2);
	elRect(recordBtn, colW*15, rowH*5.5, colW*3, rowH*2);

	sessionInput.style('font',  pxFont(colW, rowH*1.5) + ' "Courier New"');
	joinSessionBtn.style('font',  pxFont(colW, rowH) + ' "Courier New"');
	createSessionBtn.style('font',  pxFont(colW, rowH) + ' "Courier New"');
	leaveSessionBtn.style('font',  pxFont(colW, rowH) + ' "Courier New"');
	recordBtn.style('font',  pxFont(colW, rowH) + ' "Courier New"');

}

function checkEnabled() {
	if (monitorSelfToggle.checked()) {
		monitorSelfSlider.removeAttribute('disabled');
	}
	else {
		monitorSelfSlider.attribute('disabled', 'true');
	}
}

function joinSession() {
	let joinRequest = {
		"event": "join-session",
		"sessionId": sessionInput.value()
	}
	Bela.control.send(joinRequest);
}

function createSession() {
	let createRequest = {
		"event": "join-session",
		"sessionId": ""
	}
	Bela.control.send(createRequest);
}

function leaveSession() {
	let leaveRequest = {
		"event": "leave-session"
	}
	Bela.control.send(leaveRequest);
}

function record() {
	var request
	if (isRecording) {
		request = {
			"event": "stop-recording"
		}
		recordBtn.style('color', color(0));
		recordBtn.html("Record")
		isRecording = false
	}
	else {
		request = {
			"event": "start-recording"
		}
		recordBtn.style('color', color("red"));
		recordBtn.html("Recording...")
		isRecording = true
	}
	Bela.control.send(request);
}

function pxStr(val) {
	return int(val).toString()+'px'
}

function elRect(el, x, y, w, h) {
	el.position(x, y);
	el.style('width', pxStr(w));
	el.style('height', pxStr(h));
}

function fontSize(colW, rowH) {
	return Math.min(rowH/1.5, colW*2);
}

function pxFont(colW, rowH) {
	return pxStr(fontSize(colW, rowH));
}