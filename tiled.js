const fs = require('fs');
const path = require('path');
const { Renderer, FULLSCREEN, RESIZABLE } = require('./build/Release/renderer.node');
const { InputMap } = require('./input_helpers.js');

const renderer = new Renderer();

if (!renderer.initialize(800, 600, "Renderer Demo")) {
  console.error("Failed to initialize renderer");
  process.exit(1);
}

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


const tilePath = "/home/sk/workspace/js/tessera/assets/tiled/map.json"



try {
    const mapId = renderer.loadTiledMap(tilePath);
    console.log("loaded map: ", mapId)
   renderer.drawTiledMap()

    // renderer.destroyTiledMap(mapId)
    // console.log("cleaned maps")
} catch (e) {
     console.error("Error:", e.message);
}

// renderer.shutdown();