
let buf = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

let connections = [];

function setup() {
	//Create a canvas of dimensions given by current browser window
	createCanvas(windowWidth, windowHeight);

	//text font
	textFont('Courier New');

	// qualitySlider = createSlider(1, 32, 8);

	monitorSelfToggle = createCheckbox("Monitor Self", false);
	monitorSelfSlider = createSlider(1, 1000, 900);
  	monitorSelfToggle.changed(checkEnabled);

	peerInput = createInput('');
	addPeer = createButton('Connect to Peer');
	addPeer.mousePressed(addConnection);

	formatDOMElements();
	checkEnabled();
}

function drawLevelBar(x, y, maxW, maxH, avg, peak, heldPeak) {
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

function createConnection(x, y, w, h, readBuf, writeBuf) {
	let rowH = 0.25 * h;
	let colW = 0.05 * w

	let rxLevelSlider = createSlider(1, 1000, 900);
	let rxQueueSlider = createSlider(1, 32, 8);
	elRect(rxLevelSlider, colW*12, y + rowH, colW*7, rowH);
	elRect(rxQueueSlider, colW*4, y + rowH*2, colW*15, rowH);

	return {
		x: x,
		y: y,
		w: w,
		h: h,
		readBuf: readBuf,
		writeBuf: writeBuf,
		levelSlider: rxLevelSlider,
		queueSlider: rxQueueSlider
	}
}

function drawConnection(c) {
	let rowH = 0.25 * c.h;
	let colW = 0.05 * c.w;

	let name = Bela.data.buffers[c.readBuf];
	let lvlBuf = Bela.data.buffers[c.readBuf+1];
	let sendBuf = [c.queueSlider.value(), c.levelSlider.value()];

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

	Bela.data.sendBuffer(c.writeBuf, 'int', sendBuf);

	return 2; // number of read buffers read
}

function draw() {
    background(60);

	let inL = Bela.data.buffers[1];
	let inR = Bela.data.buffers[2];
	
	let rowH = 0.025 * windowHeight;
	let colW = 0.05 * windowWidth

	if (inL) {
		drawLevelBar(colW*4, rowH*1, colW*15.0, rowH*0.5, inL[0], inL[1], inL[2]);
		drawLevelBar(colW*4, rowH*1.5, colW*15.0, rowH*0.5, inR[0], inR[1], inR[2]);
	}

	fill(200);
	textSize(fontSize(colW, rowH));
	textAlign(RIGHT);
	text("Send Level:", 0, rowH*1, colW*3.5, rowH);

	let rxCount = Bela.data.buffers[9];
	if (rxCount > 0) {
		let readBufOffset = 10;
		let writeBufOffset = 1;
		let rowOffset = rowH*9;
		for (let i = 0; i < rxCount; i++) {
			if ((i + 1) > connections.length) {
				connections.push(createConnection(0, rowOffset, colW*20, rowH*4, readBufOffset, writeBufOffset));
			}
			let read = drawConnection(connections[i]);
			
			rowOffset += rowH * 4.5;
			readBufOffset += read;
			writeBufOffset += 1;
		}
	}

	// text("Receive Level:", colW, rowH*4, colW*4, rowH);
	// textAlign(CENTER);
	// text("Latency <-> Quality", colW, rowH*7, colW*18, rowH);

	// fill(50, 200, 50);

	//buf[0] = qualitySlider.value();
	buf[0] = monitorSelfToggle.checked() ? 1 : 0;
	buf[1] = monitorSelfSlider.value();
    Bela.data.sendBuffer(0, 'int', buf);
}

function formatDOMElements() {
	let rowH = 0.025 * windowHeight;
	let colW = 0.05 * windowWidth

	monitorSelfToggle.style('font',  pxFont(colW, rowH) + ' "Courier New"');
	monitorSelfToggle.style('color', color(200));
	monitorSelfToggle.style('text-align', 'right');

	elRect(monitorSelfToggle, colW, rowH*3, colW*2.5, rowH);
	elRect(monitorSelfSlider, colW*4, rowH*3, colW*15, rowH);

	elRect(peerInput, colW, rowH*6, colW*2.5, rowH);
	elRect(addPeer, colW*4, rowH*6, colW*3, rowH);

	// qualitySlider.position(colW, rowH*8);
  	// qualitySlider.style('width', int(colW*18).toString()+'px');
  	// qualitySlider.style('height', int(rowH).toString()+'px');

}

function checkEnabled() {
	if (monitorSelfToggle.checked()) {
		monitorSelfSlider.removeAttribute('disabled');
	}
	else {
		monitorSelfSlider.attribute('disabled', 'true');
	}
}

function addConnection() {
	let addRequest = {
		"event": "add-connection",
		"host": peerInput.value()
	}
	Bela.control.send(addRequest);
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