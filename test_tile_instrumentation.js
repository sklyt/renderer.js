const path = require("node:path");
const { Renderer } = require("./build/Release/renderer.node");
const { PixelBuffer } = require("tessera.js");

const frameCount = 2800;
const width = 800;
const height = 600;
const mapPath = path.resolve(__dirname, "../nexus/test/tiled/map.json");
const renderer = new Renderer();

if (!renderer.initialize(width, height, "Tile Render Instrumentation")) {
    throw new Error("Failed to initialize renderer");
}

const mapId = renderer.loadTiledMap(mapPath);
const canvas = new PixelBuffer(renderer, width, height);
const camera = {
    worldPosition: { x: 1000, y: 800 },
    worldZoom: 1,
    viewWidth: width,
    viewHeight: height,
    worldRotation: 0,
    frustum: { left: 600, right: 1400, top: 500, bottom: 1100 },
    updateWorldTransform() {}
};

renderer.initTileInstrumentation(200);
renderer.onRender(() => canvas.draw(0, 0));

let frames = 0;
function renderFrame() {
    canvas.clear(24, 28, 36);
    canvas.updateCamera(camera);
    renderer.drawTiledMap(mapId, canvas.textureId, true);
    canvas.upload();

    const running = renderer.step();
    frames++;
    if (!running || frames >= frameCount) {
        renderer.destroyTiledMap(mapId);
        renderer.shutdown();
        if (frames < frameCount) {
            console.error(`Renderer stopped after ${frames} of ${frameCount} frames`);
            process.exitCode = 1;
        }
        return;
    }

    setImmediate(renderFrame);
}

console.log(`Rendering ${frameCount} frames with chunked tile rendering`);
renderFrame();
