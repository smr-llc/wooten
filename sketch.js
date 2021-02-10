
let buf = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

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

function draw() {
    background(60);

	let inL = Bela.data.buffers[1];
	let inR = Bela.data.buffers[2];
	
	let rowH = 0.025 * windowHeight;
	let colW = 0.05 * windowWidth

    fill(20);
    noStroke();
    rect(colW*5, rowH*1, colW*10, rowH);
    // rect(colW*5, rowH*4, colW*8, rowH);

	// if (inAvgLevel >= 0) {
	// 	drawLevelBar(colW*5, rowH*4, colW*8.0, rowH, inAvgLevel, inPeakLevel, inHeldPeakLevel);
	// }
	if (inL) {
		drawLevelBar(colW*5, rowH*1, colW*10.0, rowH*0.5, inL[0], inL[1], inL[2]);
		drawLevelBar(colW*5, rowH*1.5, colW*10.0, rowH*0.5, inR[0], inR[1], inR[2]);
	}


	fill(200);
	textSize(Math.min(rowH/1.5, colW*2));
	textAlign(RIGHT);
	text("Send Level:", 0, rowH*1, colW*5, rowH);


	let rxCount = Bela.data.buffers[9];
	if (rxCount > 0) {
		let bufOffset = 10;
		let rowOffset = rowH*9;
		for (let i = 0; i < rxCount; i++) {
			let lvlBuf = Bela.data.buffers[bufOffset];

			fill(20);
			noStroke();
			rect(colW*5, rowOffset, colW*10, rowH);
			drawLevelBar(colW*5, rowOffset, colW*10.0, rowH, lvlBuf[0], lvlBuf[1], lvlBuf[2]);

			fill(200);
			textSize(Math.min(rowH/1.5, colW*2));
			textAlign(RIGHT);
			text("Receive Level:", 0, rowOffset, colW*5, rowH);

			rowOffset += rowH * 3;
			bufOffset += 1;
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

	monitorSelfToggle.position(colW, rowH*4);
	monitorSelfToggle.style('font', (rowH/1.5) + 'px "Courier New"');
	monitorSelfToggle.style('color', color(200));
	monitorSelfToggle.style('text-align', 'center');
	monitorSelfToggle.style('width', int(colW*4).toString()+'px');
	//monitorSelfToggle.style('height', int(rowH).toString()+'px');

	monitorSelfSlider.position(colW*5, rowH*4);
	monitorSelfSlider.style('width', int(colW*14).toString()+'px');
	monitorSelfSlider.style('height', int(rowH).toString()+'px');


	peerInput.position(colW, rowH*7);
	peerInput.style('width', int(colW*4.5).toString()+'px');
	peerInput.style('height', int(rowH).toString()+'px');

	addPeer.position(colW*6, rowH*7);
	addPeer.style('width', int(colW*4.5).toString()+'px');
	addPeer.style('height', int(rowH).toString()+'px');

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
