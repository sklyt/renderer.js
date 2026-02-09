const { Renderer } = require('./build/Release/renderer.node');
const { PixelBuffer } = require("tessera.js")
const renderer = new Renderer();

if (!renderer.initialize(800, 600, "Renderer Demo")) {
    console.error("Failed to initialize renderer");
    process.exit(1);
}

// Mock Camera2D
const camera = {
    worldPosition: { x: 0, y: 0 },
    targetPosition: { x: 0, y: 0 },
    velocity: { x: 0, y: 0 },
    worldZoom: 1.0,
    viewWidth: 800,
    viewHeight: 600,
    worldRotation: 0,
    frustum: {
        left: -400,
        right: 400,
        top: -300,
        bottom: 300
    },
    dirty: false,

    setPosition(x, y) {
        this.targetPosition.x = x;
        this.targetPosition.y = y;
        this.dirty = true;
    },

    updateWorldTransform() {
        if (!this.dirty) return;

        // Lerp towards target position
        const lerpFactor = 0.1;
        this.worldPosition.x += (this.targetPosition.x - this.worldPosition.x) * lerpFactor;
        this.worldPosition.y += (this.targetPosition.y - this.worldPosition.y) * lerpFactor;

        // Calculate half dimensions adjusted by zoom
        const halfWidth = (this.viewWidth / 2) / this.worldZoom;
        const halfHeight = (this.viewHeight / 2) / this.worldZoom;

        // Update frustum relative to world position
        this.frustum.left = this.worldPosition.x - halfWidth;
        this.frustum.right = this.worldPosition.x + halfWidth;
        this.frustum.top = this.worldPosition.y - halfHeight;
        this.frustum.bottom = this.worldPosition.y + halfHeight;

        this.dirty = false;
    }
};


const atlasId = renderer.loadAtlas("/home/sk/workspace/js/tessera/assets/G_GqXDrXwAAi5Bi.jpg");
console.log("Loaded atlas:", atlasId);

// We can load multiple atlases
const atlas2 = renderer.loadAtlas("/home/sk/workspace/js/tessera/assets/G_GqXDrXwAAi5Bi.jpg");
console.log("Loaded atlas 2:", atlas2);
// 
console.log(renderer.isAtlasOpaque(atlasId))
// console.log(renderer.getAtlasPixel(atlasId, 0, 0))
// console.log(renderer.getAtlasDataAndFree(atlasId))
// console.log(renderer.freeAtlas(atlas2))
const spriteId = renderer.createSprite(atlas2, 32, 32, 8, true);
const spriteId2 = renderer.createSpriteWithAnimations(
    atlas2,
    32, 32,
    {
        idle: { frames: [0, 1], fps: 4, loop: true },
        walk: { frames: [2, 3, 4, 5], fps: 8, loop: true },
        jump: { frames: [6, 7], fps: 10, loop: false }
    },
    true // opaque
);

const animatorId = renderer.createAnimator({
    up: {
        frames: [
            "./assets/playerGrey_up1.png",
            "./assets/playerGrey_up2.png"
        ],
        fps: 6,
        loop: true
    },
    walk: {
        frames: [
            "./assets/playerGrey_walk1.png",
            "./assets/playerGrey_walk2.png"
        ],
        fps: 6,
        loop: true
    }
});

console.log("animator id: ", animatorId)

// // Update transform
// renderer.updateAnimator(animatorId, x, y, rotation, scaleX, scaleY, flipH, flipV);

// // Play animation
// renderer.playAnimatorAnimation(animatorId, "walk");

// // Update timing
// renderer.updateAnimators(delta);

// // Draw
// renderer.drawAnimator(animatorId, canvas.textureId);

console.log("Created sprite:", spriteId);
renderer.updateSprite(
    spriteId,
    500,    // x
    500,    // y
    0,      // rotation
    1.0,    // scaleX
    1.0,    // scaleY
    0,      // frame
    false,  // flipH
    false   // flipV
);

renderer.updateSprite(
    spriteId2,
    0,    // x
    0,    // y
    0,      // rotation
    2.0,    // scaleX
    2.0,    // scaleY
    0,      // frame
    false,  // flipH
    false   // flipV
);
renderer.playAnimation(spriteId2, "idle");
console.log("Updated sprite position to (400, 300)");
const sprite2 = renderer.createSprite(atlasId, 32, 32, 8, true);
const sprite3 = renderer.createSprite(atlasId, 32, 32, 8, true);
console.log("Created sprites 2 and 3:", sprite2, sprite3);
// Cleanup
// renderer.destroySprite(spriteId);
// renderer.destroySprite(sprite2);
// renderer.destroySprite(sprite3);
console.log("Destroyed all sprites");

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
let frame = 0;
let lastTime = performance.now();
function loop() {
    const startTime = performance.now();
    const now = performance.now();
    const delta = (now - lastTime) / 1000.0; // Convert to seconds
    lastTime = now;

    // Update animations (C++ advances frames)
    renderer.updateSpriteAnimations(delta);
    clear(50, 50, 50, canvas.data.buffer); // JS writes background
    canvas.updateCamera(camera);
    const buffer = canvas.getCurrentBuffer();
    const idx = (10 * canvas.width + 10) * 4;
    buffer[idx] = 255;
    buffer[idx + 1] = 255;
    buffer[idx + 2] = 255;
    buffer[idx + 3] = 255;
    canvas.markRegion(10, 10, 1, 1);
    cppCommand(canvas); // C++ processes + writes


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
    renderer.drawSprite(spriteId, canvas.textureId);
    renderer.drawSprite(spriteId2, canvas.textureId);

    // Animate
    frame = (frame + 1) % 8;
    renderer.updateSprite(spriteId, -100, 100, 0, 2.0, 2.0, frame, false, false);
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