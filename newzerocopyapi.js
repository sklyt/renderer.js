const fs = require('fs');
const path = require('path');
const { Renderer, FULLSCREEN, RESIZABLE } = require('./build/Release/renderer.node');
const { InputMap } = require('./input_helpers.js');

const renderer = new Renderer();

if (!renderer.initialize(800, 600, "Renderer Demo")) {
    console.error("Failed to initialize renderer");
    process.exit(1);
}

console.log("Init shared", renderer.initSharedBuffers)
const width = 800
const height = 600
const bufferSize = width * height * 4;
const buffers = [
    new ArrayBuffer(bufferSize),
    new ArrayBuffer(bufferSize),
    new ArrayBuffer(bufferSize)
];


const MAX_DIRTY_REGIONS = 256;
const CTRL_JS_WRITE_IDX = 10;
// control buffer size in bytes
const CONTROL_BUFFER_SIZE = (10 + 5 + MAX_DIRTY_REGIONS * 4) * 4;  // 4116 bytes

const controlBuffer = new Uint32Array(new ArrayBuffer(CONTROL_BUFFER_SIZE));
const bufferId = renderer.initSharedBuffers(buffers[0],
    buffers[1],
    buffers[2],
    controlBuffer.buffer,
    width,
    height);
const views = [
    new Uint8ClampedArray(buffers[0]),
    new Uint8ClampedArray(buffers[1]),
    new Uint8ClampedArray(buffers[2])
];




let rectx = 100;

const clear = (r, g, b, a = 255, buffer = undefined) => {
    // const buffer = this.getCurrentBuffer();

    const startTime = performance.now();


    // Pack RGBA into a single 32-bit value: (A << 24) | (B << 16) | (G << 8) | R
    const color = ((a & 0xFF) << 24) | ((b & 0xFF) << 16) | ((g & 0xFF) << 8) | (r & 0xFF);


    // const uint32View = new Uint32Array(this.data.buffer); old way 
    const uint32View = new Uint32Array(buffer); // new way

    // Native fill - much faster than JS loop
    uint32View.fill(color);

    // this.dirtyTracker.addRegion(0, 0, this.width, this.height); // proc the control buffer dirty region count
    // this.dirtyTracker.markDirty()
    // this.renderer.updateBufferData(this.bufferId, this.data);
    // this.needsUpload = true;

    const elapsed = performance.now() - startTime;
    // if (this.DEBUG)
    console.log(`Clear (${width}x${height}): ${elapsed.toFixed(2)}ms`);
}


renderer.onRender(() => {
    renderer.clear({ r: 0.1, g: 0.12, b: 0.15, a: 1 });
    renderer.drawRectangle({ x: rectx, y: 100 }, { x: 200, y: 150 }, { r: 1, g: 0, b: 0, a: 1 });

    renderer.drawTexture(bufferId, { x: 0, y: 0 });
});

// main loop
function Loop() {
    renderer.input.GetInput();
    const idx = Atomics.load(controlBuffer, CTRL_JS_WRITE_IDX);
    clear(255, 255, 255, 255, views[idx].buffer)
    Atomics.store(controlBuffer, 13, 1);
    if (renderer.step()) setImmediate(Loop);
    else {
        console.log('renderer loop ended');
        renderer.shutdown();
    }
}
Loop();

process.on('SIGINT', () => {
    console.log('SIGINT -> shutdown');
    renderer.shutdown();
    process.exit(0);
});
