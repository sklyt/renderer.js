const { Renderer } = require('./build/Release/renderer.node');
const { PixelBuffer } = require("tessera.js")
const renderer = new Renderer();

if (!renderer.initialize(800, 600, "Renderer Demo")) {
    console.error("Failed to initialize renderer");
    process.exit(1);
}



const atlasId = renderer.loadAtlas("/home/sk/workspace/js/tessera/assets/G_GqXDrXwAAi5Bi.jpg");
console.log("Loaded atlas:", atlasId);

// We can load multiple atlases
const atlas2 = renderer.loadAtlas("/home/sk/workspace/js/tessera/assets/G_GqXDrXwAAi5Bi.jpg");
console.log("Loaded atlas 2:", atlas2);
// 
console.log(renderer.isAtlasOpaque(atlasId))
console.log(renderer.getAtlasPixel(atlasId, 0, 0))
console.log(renderer.getAtlasDataAndFree(atlasId))
console.log(renderer.freeAtlas(atlas2))

// Invalid path
try {
    const bad = renderer.loadAtlas("./404.png");
} catch (e) {
    console.log("Expected error:", e.message);
}

const canvas = new PixelBuffer(renderer, 800, 600);

// Simulate a C++ command that writes to the buffer
function cppCommand() {
    // First: process what JS wrote
    renderer.processPendingRegions(canvas.textureId);

    // Now C++ writes directly to the same buffer
    const buffer = canvas.getCurrentBuffer();
    const w = canvas.width;

    // Draw a red square (simulate sprite blit)
    for (let y = 100; y < 150; y++) {
        for (let x = 100; x < 150; x++) {
            const idx = (y * w + x) * 4;
            buffer[idx] = 255;     // R
            buffer[idx + 1] = 0;   // G
            buffer[idx + 2] = 0;   // B
            buffer[idx + 3] = 255; // A
        }
    }

    // Mark this region dirty for next swap
    canvas.markRegion(100, 100, 50, 50);

}

renderer.onRender(() => {
    canvas.draw(0, 0)
});


const clear = (r, g, b, a = 255, buffer = undefined) => {
    // const buffer = this.getCurrentBuffer();




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


}

function loop() {
    const startTime = performance.now();
    clear(50, 50, 50, canvas.data.buffer); // JS writes background

    cppCommand(canvas); // C++ processes + writes

    // JS continues writing (green square)
    const buffer = canvas.getCurrentBuffer();
    const w = canvas.width;
    for (let y = 200; y < 250; y++) {
        for (let x = 200; x < 250; x++) {
            const idx = (y * w + x) * 4;
            buffer[idx] = 0;
            buffer[idx + 1] = 255;
            buffer[idx + 2] = 0;
            buffer[idx + 3] = 255;
        }
    }
    canvas.markRegion(200, 200, 50, 50);

    cppCommand(canvas); // C++ processes again

    // JS writes one more thing (blue square)
    for (let y = 300; y < 350; y++) {
        for (let x = 300; x < 350; x++) {
            const idx = (y * w + x) * 4;
            buffer[idx] = 0;
            buffer[idx + 1] = 0;
            buffer[idx + 2] = 255;
            buffer[idx + 3] = 255;
        }
    }
    canvas.markRegion(300, 300, 50, 50);

    // NOW we swap and upload everything
    canvas.upload();
    const elapsed = performance.now() - startTime;
    // if (this.DEBUG)
    console.log(`Clear (${800}x${600}): ${elapsed.toFixed(2)}ms`);

    if (renderer.step()) {
        setImmediate(loop);
    } else {
        renderer.shutdown();
    }
}

loop();