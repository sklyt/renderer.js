# tessera.js

Node.js graphics library that exposes a shared framebuffer between JavaScript
and C++. Fill it with pixel data (noise, images, textures), pass it to C++,
raylib renders it.


- [tessera.js](#tesserajs)
  * [Installation](#installation)
  * [Core Concepts](#core-concepts)
  * [Quick Start](#quick-start)
  * [Examples](#examples)
    + [Rendering Simplex Noise](#rendering-simplex-noise)
    + [Animation](#animation)
    + [Input Handling](#input-handling)
  * [API Reference](#api-reference)
    + [Pixel Buffer](#pixel-buffer)
    + [Primitives](#primitives)
      - [Lines](#lines)
        * [Anti-aliased](#anti-aliased)
      - [Shapes](#shapes)
      - [Polygons](#polygons)
      - [Camera](#camera)
      - [Font](#font)
    + [Input](#input)
    + [Image](#image)
    + [Sprites and Spritesheets](#sprites-and-spritesheets)
    + [Sound](#sound)
    + [Utils](#utils)
      - [Window & Monitor Management](#window--monitor-management)
      - [Cursor Control](#cursor-control)
      - [Utilities](#utilities)
  * [License](#license)


## Installation

```bash
npm i tessera.js
```

## Core Concepts

**Frame Buffer** - raw pixels that map to a screen

```js
import { PixelBuffer } from "tessera.js"; // raw memory
```

Every abstraction in this library builds from this primitive.

## Quick Start

```js
import { Renderer, PixelBuffer, CacheBuffer } from "tessera.js";

const [renderer, ops] = Renderer.create("Image", 650, 400)


renderer.targetFPS = 60;

const canvas = new PixelBuffer(renderer, 650, 400); // where every pixel goes
const cache = new CacheBuffer(650, 400)
cache.clear(1, 1, 1, 255)

const data = cache.data; // "pointer" to raw buffer for direct memory access
const width = canvas.width; // prefer variables outside loop, constant access in tight loop can be bad
const height = canvas.height;


for (let i = 0; i < 2000; i++) {
    const x = Math.floor(Math.random() * width);
    const y = Math.floor(Math.random() * height);
    const brightness = 100 + Math.random() * 155;
    const idx = (y * width + x) * 4; // formula from screen(x, y) to buffer(linear memory)
    data[idx] = brightness;
    data[idx + 1] = brightness;
    data[idx + 2] = brightness;

}


renderer.onRender(() => {
    canvas.draw(0, 0); // draw at 0,0 top left

    // draw on the window directly (see font to draw on the canvas)
    renderer.drawText(
        `FPS: ${renderer.FPS} | Buffer: ${canvas.width}x${canvas.height} | Memory: ${(canvas.width * canvas.height * 4 / 1024).toFixed(1)}KB`,
        { x: 20, y: 750 },
        16,
        { r: 1, g: 1, b: 1, a: 1 }
    );
});

function Loop() {
    renderer.input.GetInput(); // <- get keyboard/mouse input state
    canvas.blit(cache) // apply cache
    canvas.upload() // apply to texture
    if (renderer.step()) { // calls the callback and draws on screen
        setImmediate(Loop);
    } else {
        console.log("loop ended");
        renderer.shutdown();
    }
}

Loop(); // non-blocking because step runs in c++, not js

process.on("SIGINT", () => {
    console.log("\nshutting down gracefully...");
    renderer.shutdown();
    process.exit(0);
});
```

## Examples

### Rendering Simplex Noise

```js

function generateSimplexNoise(data, width, height, options = {}) {
    // simplified Simplex-like noise
    const scale = options.scale || 0.01;

    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            const idx = (y * width + x) * 4;

            // Simple gradient noise approximation
            const value = Math.sin(x * scale) * Math.cos(y * scale);
            const normalized = (value + 1) * 0.5 * 255;

            data[idx] = normalized; // R
            data[idx + 1] = normalized; // G
            data[idx + 2] = normalized; // B
            data[idx + 3] = 255; // A
        }
    }

}

generateSimplexNoise(cache.data, canvas.width, canvas.height);


renderer.onRender(() => {
    canvas.draw(0, 0);
});
```

### Animation

Concepts are the same all you do is clear the canvas every frame

```js
import { LineDrawer } from "tessera.js"; // line drawing utility handles tracking and flushing
let animationTime = 0;
function demoBasicLines() {
    canvas.clear(20, 25, 35, 255);

    //different line slopes
    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const radius = 150;

    // Radial lines showing all angles
    for (let i = 0; i < 360; i += 15) {
        const angle = (i * Math.PI) / 180;
        const endX = centerX + Math.cos(angle) * radius;
        const endY = centerY + Math.sin(angle) * radius;

        // Color based on angle
        const r = Math.floor(128 + 127 * Math.sin(angle));
        const g = Math.floor(128 + 127 * Math.sin(angle + Math.PI * 2 / 3));
        const b = Math.floor(128 + 127 * Math.sin(angle + Math.PI * 4 / 3));

        LineDrawer.drawLine(canvas, centerX, centerY, endX, endY, r, g, b, 255);
    }

    // thick lines demonstration
    const thickY = 50;
    for (let thickness = 1; thickness <= 8; thickness++) {
        const x = 50 + thickness * 70;
        LineDrawer.drawThickLine(
            canvas,
            x,
            thickY,
            x,
            thickY + 250,
            thickness,
            255,
            200,
            100,
            255,
        );
    }

    // Animated line
    const animatedAngle = animationTime * 2;
    const animX = centerX + Math.cos(animatedAngle) * 100;
    const animY = centerY + Math.sin(animatedAngle) * 100;

    LineDrawer.drawThickLine(
        canvas,
        centerX,
        centerY,
        animX,
        animY,
        3,
        255,
        255,
        255,
        255,
    );

    canvas.upload();
}
renderer.onRenderer(() => {
    animationTime += 0.016; // Roughly 60 FPS
});
function Loop() {
    renderer.input.GetInput();
    demoBasicLines();
    if (renderer.step()) {
        setImmediate(Loop);
    } else {
        console.log("loop ended");
        renderer.shutdown();
    }
}
```

### Input Handling

```js
import { InputMap, loadRenderer, ShapeDrawer } from "tessera.js";

// helper class to capture input in a sane way
const inputMap = new InputMap(renderer.input);

// map actions to keys (custom name, array of actual keys)
inputMap.mapAction("move_left", ["A", "ArrowLeft"]);
inputMap.mapAction("move_right", ["D", "ArrowRight"]);
inputMap.mapAction("move_up", ["W", "ArrowUp"]);
inputMap.mapAction("move_down", ["S", "ArrowDown"]);
inputMap.mapAction("jump", ["Space"]);
inputMap.mapAction("shoot", ["Enter"]);

let rectx = 100;
let recty = 100;

renderer.onRender(() => {
    canvas.clear(20, 25, 35, 255);
    ShapeDrawer.fillRect(canvas, rectx, recty, 280, 100, 40, 45, 55, 200);
    canvas.upload();
    canvas.draw(0, 0);
});

function Loop() {
     renderer.input.GetInput(); // poll input

    if (inputMap.isActionActive("move_left")) {
        rectx -= 10;
    }
    if (inputMap.isActionActive("move_right")) {
        rectx += 10;
    }

    if (renderer.step()) {
        setImmediate(Loop);
    } else {
        renderer.shutdown();
    }
}

Loop();
```

## API Reference

### Pixel Buffer

```js
const canvas = new PixelBuffer(renderer, width, height);
canvas.coordToIndex(x, y); // convert 2D coordinate to 1D memory index
canvas.setPixel(x, y, r, g, b, a = 255); // don't use in hot paths prefer direct memory access, this calls c++ on every call compared to direct access
canvas.getPixel(x, y); // => {r, g, b, a}
canvas.clear(r, g, b, a = 255);
canvas.upload(); // commit buffer changes
canvas.draw(x, y); // draw this buffer to the screen at specified position
canvas.grow(newWidth, newHeight); //  If the requested size is smaller or equal, this is a no-op.
canvas.destroy();
```

### Primitives

#### Lines

```js
import { LineDrawer } from "tessera.js";

// draw a 1px Bresenham line.
LineDrawer.drawLine(canvas, x1, y1, x2, y2, r, g, b, a = 255, camera = undefined,
);

// draw a thick line by stamping a precomputed circular brush along Bresenham points.
LineDrawer.drawThickLine(
    canvas, x1, y1, x2, y2, thickness, r, g, b, a = 255, camera = undefined,
);
```

##### Anti-aliased

```js
import { AADrawer } from "tessera.js";

AADrawer.drawLineAA(
    canvas, x1, y1,  x2, y2, r, g,b, a = 255,
    camera = undefined,
);
AADrawer.drawCircleAA( canvas, cx, cy, radius, r, g, b, a = 255,
    camera = undefined,
);
AADrawer.fillCircleAA(canvas, cx, cy,  radius,  r,  g,  b, a = 255,
    camera = undefined,
);
```

#### Shapes

```js
import { ShapeDrawer } from "tessera.js";

ShapeDrawer.fillRect(
    canvas, x,  y, width, height, r, g, b, a = 255,
    camera = undefined,
);
ShapeDrawer.strokeRect(
    canvas, x, y, width, height, thickness, r, g, b, a = 255,
    camera = undefined,
);
ShapeDrawer.fillCircle(
    canvas, cx, cy, radius, r, g, b, a = 255,
    camera = undefined,
);
ShapeDrawer.strokeCircle(
    canvas, cx, cy, radius, thickness, r, g, b, a = 255,
    camera = undefined,
);
```

#### Polygons

```js
import { PolygonDrawer } from "tessera.js";

// Create common polygon shapes (return array of points [{x,y}])
PolygonDrawer.createRegularPolygon(cx, cy, radius, sides);
PolygonDrawer.createStar(cx, cy, outerRadius, innerRadius, points);
PolygonDrawer.createRoundedRect(x, y, width, height, radius);

PolygonDrawer.fillPolygon(canvas, points, r, g, b, a = 255); // Fill a polygon using scanline edge-table (handles concave / self-intersecting).
PolygonDrawer.strokePolygon(canvas, points, thickness, r, g, b, a = 255);
```

#### Camera

```js
import { Camera } from "tessera.js";
// camera - world <-> screen transforms
const cam = new Camera(x = 0, y = 0, width = 800, height = 600);
cam.worldToScreen(worldX, worldY); // => { x, y } (applies viewport if set)
cam.screenToWorld(screenX, screenY); // => { x, y } (inverse, considers viewport)
cam.setViewport(viewport); // attach a Viewport
cam.isVisible(worldX, worldY); // => boolean
cam.getVisibleBounds(); // => { left, right, top, bottom }
cam.move(dx, dy);
cam.setPosition(x, y);

// viewport - canvas sub-region / scissor
const vp = new Viewport(x, y, width, height);
vp.contains(screenX, screenY); // => boolean
vp.toCanvas(localX, localY); // => { x, y }   // local -> canvas coords
vp.toLocal(canvasX, canvasY); // => { x, y }   // canvas -> local coords
vp.getAspectRatio(); // => number
vp.setScissor(enabled); // enable/disable scissor clipping
vp.shouldClip(canvasX, canvasY); // => boolean (true if pixel is outside viewport)
```

#### Font

```
// BitmapFont - simple API (bitmap atlas text rendering)
// Constructor
// new BitmapFont(renderer, atlasPath, config = {})
// config keys: bitmapWidth, bitmapHeight, cellsPerRow, cellsPerColumn,
// cellWidth, cellHeight, fontSize, offsetX, offsetY, charOrder,
// lineHeight, lineGap, charSpacing
// use this website: https://lucide.github.io/Font-Atlas-Generator/
// the default char order follows it:
/**
 * getDefaultCharOrder() {
        return ' ☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼' +
            ' !"#$%&\'()*+,-./0123456789:;<=>?' +
            '@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_' +
            '`abcdefghijklmnopqrstuvwxyz{¦}~⌂' +
            'ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥₧ƒ' +
            'áíóúñÑªº¿⌐¬½¼¡«»░▒▓│┤╡╢╖╕╣║╗╝╜╛┐' +
            '└┴┬├─┼╞╟╚╔╩╦╠═╬╧╨╤╥╙╘╒╓╫╪┘┌█▄▌▐▀' +
            'αßΓπΣσµτΦΘΩδ∞φε∩≡±≥≤⌠⌡÷≈°∙·√ⁿ²■□';
    }
 **/
const font = new BitmapFont(renderer, "fonts/atlas.png", {
    cellWidth: 32,
    cellHeight: 32,
});



// Properties
font.renderer; // Renderer instance used to load atlas
font.atlasImage; // { data: Uint8Array, width, height }
font.config; // resolved config object:
           this.config = {
            bitmapWidth: config.bitmapWidth || this.atlasImage.width,
            bitmapHeight: config.bitmapHeight || this.atlasImage.height,
            cellsPerRow: config.cellsPerRow || 16,
            cellsPerColumn: config.cellsPerColumn || 16,
            cellWidth: config.cellWidth || 32,
            cellHeight: config.cellHeight || 32,
            fontSize: config.fontSize || 16,
            offsetX: config.offsetX || 0,
            offsetY: config.offsetY || 0,
            charOrder: config.charOrder || this.getDefaultCharOrder(),
            lineHeight: config.lineHeight || config.cellHeight,
            lineGap: config.lineGap || 0,      // Extra space between lines
            charSpacing: config.charSpacing || 0  // Extra space between characters
        };
  
font.glyphs; // Map<char, Glyph>
font.lineHeight; // number
font.baseline; // number
// Glyph (internal structure)
class Glyph {
    // fields: char, x, y, width, height, xOffset, yOffset, xAdvance
}
// Lookup, measurement & drawing
font.getGlyph(char); // => Glyph (with fallback)
font.measureText(text); // => { width: number, height: number }
font.drawText(canvas, text, x, y, color = { r: 255, g: 255, b: 255, a: 255 }, scale = 1.0, camera = undefined); // draws text into PixelBuffer (alpha blended). Commits minimal region and sets canvas.needsUpload to true
font.drawMultilineText(
    canvas,
    text,
    x,
    y,
    maxWidth,
    align = "left",
    color = { r: 255, g: 255, b: 255, a: 255 },
    scale = 1.0,
    camera = undefined,
); // draws wrapped lines; returns { width, height }
font.drawTextWithTint(canvas, text, x, y, r, g, b, a = 255, scale = 1.0, camera = undefined);

Updated API, with rotation examples:

// Regular text
font.drawText(canvas, "Hello", 100, 100);

// Rotated text (45 degrees)
font.drawText(canvas, "Rotated!", 200, 200, {r:255, g:0, b:0, a:255}, 1.0, 45, camera);

// Scaled and rotated
font.drawText(canvas, "Big!", 300, 300, {r:0, g:255, b:0, a:255}, 2.0, -30, camera);
```

### Input

```js
import {InputMap} from "tessera.js"

// InputMap - action mapping & callback helpers
const im = new InputMap(renderer.input)

/**
 * key mappings:
 * void InputManager::InitializeKeyMappings()
{
    // Letters
    for (char c = 'A'; c <= 'Z'; c++)
    {
        keyNameToCode_[std::string(1, c)] = c;
        keyCodeToName_[c] = std::string(1, c);
    }

    // Numbers
    for (char c = '0'; c <= '9'; c++)
    {
        keyNameToCode_[std::string(1, c)] = c;
        keyCodeToName_[c] = std::string(1, c);
    }

    // Special keys
    keyNameToCode_["Space"] = KEY_SPACE;
    keyCodeToName_[KEY_SPACE] = "Space";

    keyNameToCode_["Enter"] = KEY_ENTER;
    keyCodeToName_[KEY_ENTER] = "Enter";

    keyNameToCode_["Escape"] = KEY_ESCAPE;
    keyCodeToName_[KEY_ESCAPE] = "Escape";

    keyNameToCode_["Backspace"] = KEY_BACKSPACE;
    keyCodeToName_[KEY_BACKSPACE] = "Backspace";

    keyNameToCode_["Tab"] = KEY_TAB;
    keyCodeToName_[KEY_TAB] = "Tab";

    keyNameToCode_["Shift"] = KEY_LEFT_SHIFT;
    keyCodeToName_[KEY_LEFT_SHIFT] = "Shift";

    keyNameToCode_["Control"] = KEY_LEFT_CONTROL;
    keyCodeToName_[KEY_LEFT_CONTROL] = "Control";

    keyNameToCode_["Alt"] = KEY_LEFT_ALT;
    keyCodeToName_[KEY_LEFT_ALT] = "Alt";

    // Arrow keys
    keyNameToCode_["ArrowUp"] = KEY_UP;
    keyCodeToName_[KEY_UP] = "ArrowUp";

    keyNameToCode_["ArrowDown"] = KEY_DOWN;
    keyCodeToName_[KEY_DOWN] = "ArrowDown";

    keyNameToCode_["ArrowLeft"] = KEY_LEFT;
    keyCodeToName_[KEY_LEFT] = "ArrowLeft";

    keyNameToCode_["ArrowRight"] = KEY_RIGHT;
    keyCodeToName_[KEY_RIGHT] = "ArrowRight";

    // Function keys
    for (int i = 1; i <= 12; i++)
    {
        std::string name = "F" + std::to_string(i);
        int keyCode = KEY_F1 + (i - 1);
        keyNameToCode_[name] = keyCode;
        keyCodeToName_[keyCode] = name;
    }
}
 * */

im.mapAction(actionName, keys)        // map keyboard keys (string|[string]) -> action e.g im.mapAction("move_left", ["A", "ArrowLeft"]);
im.MapMouseAction(actionName, keys)   // map mouse buttons (string|[string]) -> action  e.g im.mapaction("left_click", [0]) 

im.isMouseActionActive(actionName)    // => boolean (mouse button down)
im.isMousePressed(actionName)         // => boolean (mouse button pressed)
im.IsMouseActionReleased(actionName)  // => boolean (mouse button released)

im.mousePosition                      // getter => { x:number, y:number }
im.mouseDelta                         // getter => { x:number, y:number }
im.mouseWheelDelta                    // getter => number

im.isActionActive(actionName)         // => boolean (key down)
im.wasActionTriggered(actionName)     // => boolean (key pressed)  // note: implementation uses isKeyDown
im.isActionReleased(actionName)       // => boolean (key released)

im.onActionDown(actionName, callback) // register callback on mapped key down; returns nothing (stores callback ids)
im.onActionUp(actionName, callback)   // register callback on mapped key up
im.onMouseDown(actionName, callback)  // register callback on mapped mouse down
im.onMouseUp(actionName, callback)    // register callback on mapped mouse up
im.onMouseMove(callback)              // register mouse-move callback (event)
im.onMouseWheel(callback)             // register mouse-wheel callback (event)

im.cleanup()                          // remove all registered callbacks

// callback signature used:
// (actionName, event) or (event) where event = {
//   type, keyCode, keyName, mouseButton,
//   mousePosition: {x,y}, mouseDelta: {x,y},
//   wheelDelta, timestamp
// }
```

### Image

```js
renderer.loadImage(path) // @returns {width: number, height: number, format: number, data: Uint8Array}

import {drawAtlasRegionToCanvas, imageToCanvas} from "tessera.js"
/**
 * 
 * @param {{data: Uint8Array, width: number, height: number}} atlas 
 * @param {{x: number, y: number, width: number, height: number}} srcRect 
 * @param {{data: Uint8Array, width: number, height: number}} canvas 
 * @param {{x: number, y: number, width: number, height: number}} destRect 
 * @param {"bilinear" | "nn"} algorithm - The resizing algorithm: 'bi' for bilinear interpolation or 'nn' for nearest neighbor. Defaults to 'bi'.
 */
  drawAtlasRegionToCanvas(atlas, srcRect, canvas, destRect, algorithm = 'bilinear')

   /**
 * 
 * @param {{data: Uint8Array, width: number, height: number}} img 
 * @param {{data: Uint8Array, width: number, height: number}} canvas 
 * @param {"bilinear" | "nn"} algorithm - The resizing algorithm: 'bilinear' for bilinear interpolation or 'nn' for nearest neighbor. Defaults to 'bi'.
 * @param {number} destWidth - The destination width. Defaults to canvas.width.
 * @param {number} destHeight - The destination height. Defaults to canvas.height.
 */
   imageToCanvas(img, canvas, algorithm = 'bilinear', destWidth = canvas.width, destHeight = canvas.height)



export function genFrames(rows, cols, startX, startY, tileSize) // utility for spritesheet to frames
```

### Sprites and Spritesheets

Load and manage sprite atlases efficiently with direct pixel access and memory control.

```js
// Load a sprite atlas from a file path
const atlasId = renderer.loadAtlas(imagePath)
// @returns {number} atlasId - unique identifier for the loaded atlas (0 on failure)

// Get a single pixel from the atlas
const pixel = renderer.getAtlasPixel(atlasId, x, y)
// @param {number} atlasId - atlas identifier
// @param {number} x - pixel x coordinate
// @param {number} y - pixel y coordinate
// @returns {number} packed RGBA pixel value (0 on out of bounds)

// Check if the entire atlas is fully opaque (no transparency)
const isOpaque = renderer.isAtlasOpaque(atlasId)
// @param {number} atlasId - atlas identifier
// @returns {boolean} true if all pixels have full alpha (255), false if any transparent

// Get atlas pixel data without freeing
const data = renderer.getAtlasData(atlasId)
// @param {number} atlasId - atlas identifier
// @returns {{width: number, height: number, data: Uint8Array}} atlas metadata and RGBA pixel data

// Get atlas data and automatically free resources
const data = renderer.getAtlasDataAndFree(atlasId)
// @param {number} atlasId - atlas identifier
// @returns {{width: number, height: number, data: Uint8Array}} atlas metadata and RGBA pixel data
// NOTE: atlas is freed after data is copied, handle is invalid after call

// Manually free an atlas from memory
renderer.freeAtlas(atlasId)
// @param {number} atlasId - atlas identifier to free
// NOTE: use this after loading if not needed, or let them persist for rendering

// Create an animated sprite from an atlas
const spriteId = renderer.createSprite(atlasId, frameWidth, frameHeight, frameCount, opaque)
// @param {number} atlasId - atlas identifier to use
// @param {number} frameWidth - width of each frame in pixels
// @param {number} frameHeight - height of each frame in pixels
// @param {number} frameCount - total number of frames in the atlas
// @param {boolean} opaque - optional, whether the sprite has no transparency (default: false)
// @returns {number} spriteId - unique identifier for the sprite (0 on failure)

// Update sprite state (position, rotation, scale, frame, flipping)
renderer.updateSprite(spriteId, x, y, rotation, scaleX, scaleY, frame, flipH, flipV)
// @param {number} spriteId - sprite identifier
// @param {number} x - world position x
// @param {number} y - world position y
// @param {number} rotation - rotation in radians
// @param {number} scaleX - horizontal scale (1.0 = original size)
// @param {number} scaleY - vertical scale (1.0 = original size)
// @param {number} frame - current frame index (0 to frameCount-1)
// @param {boolean} flipH - optional, flip horizontally (default: false)
// @param {boolean} flipV - optional, flip vertically (default: false)

// Draw a sprite to the screen or render target
renderer.drawSprite(spriteId, bufRefId)
// @param {number} spriteId - sprite identifier
// @param {number} bufRefId - buffer reference to draw to (0 = screen)
// NOTE: sprite must be updated with updateSprite before drawing

// Destroy a sprite and free resources
renderer.destroySprite(spriteId)
// @param {number} spriteId - sprite identifier to destroy
```

**Example: Loading and using a sprite atlas**

```js
// Load atlas
const atlasId = renderer.loadAtlas("./sprites/characters.png");
if (atlasId === 0) {
    console.error("Failed to load atlas");
    process.exit(1);
}

// Check properties
const data = renderer.getAtlasData(atlasId);
console.log(`Atlas: ${data.width}x${data.height} (${data.data.byteLength} bytes)`);

const opaque = renderer.isAtlasOpaque(atlasId);
console.log(`Opaque: ${opaque}`);

// Sample a pixel at (0, 0)
const pixelColor = renderer.getAtlasPixel(atlasId, 0, 0);
const r = (pixelColor & 0xFF);
const g = (pixelColor & 0xFF00) >> 8;
const b = (pixelColor & 0xFF0000) >> 16;
const a = (pixelColor & 0xFF000000) >> 24;
console.log(`Pixel at (0,0): RGBA(${r}, ${g}, ${b}, ${a})`);

// When done, free the memory
renderer.freeAtlas(atlasId);
```

**Example: Load, process, and free in one call**

```js
// Useful when you need the data temporarily (e.g., for processing or uploading to GPU)
const atlasData = renderer.getAtlasDataAndFree(atlasId);

// Process the data...
processAtlasData(atlasData);

// atlasId is now invalid - automatically freed
```

### Sound

```
loadSound(filePath)               // => SoundHandle (number) | 0 on failure
                                  // example: const h = renderer.audio.loadSound("/path/sfx.wav");

loadSoundFromMemory(fileType, buffer)
                                  // fileType: ".wav"|".ogg"|".mp3" etc
                                  // buffer: Node Buffer | ArrayBuffer | TypedArray | SharedArrayBuffer
                                  // => SoundHandle | 0 on failure
                                  // example: const buf = fs.readFileSync("click.wav"); renderer.audio.loadSoundFromMemory(".wav", buf);

playSound(handle)                 // play a loaded sound (SFX)
stopSound(handle)                 // stop playback of sound
pauseSound(handle)                // pause sound
resumeSound(handle)               // resume paused sound
setSoundVolume(handle, volume)    // volume 0..1 (multiplied by masterVolume)
isSoundPlaying(handle)            // => boolean
unloadSound(handle)               // free sound resources

loadMusic(filePath)               // => MusicHandle | 0 on failure (streamed)
loadMusicFromMemory(fileType, buffer)
                                  // => MusicHandle | 0 on failure (streamed from bytes)
playMusic(handle)                 // start playing streamed music
stopMusic(handle)                 // stop and reset music stream
pauseMusic(handle)                // pause stream
resumeMusic(handle)               // resume stream
setMusicVolume(handle, volume)    // volume 0..1 (multiplied by masterVolume)
isMusicPlaying(handle)            // => boolean
unloadMusic(handle)               // free stream resources

setMasterVolume(volume)           // global master volume 0..1 (affects created sounds/music)
getMasterVolume()                 // => number

Notes:
- load*FromMemory accepts Node Buffers, ArrayBuffers, TypedArrays, SharedArrayBuffers.
- All load* functions return 0 on failure. Check result before calling play*.
- For music streams you should call renderer.step() or your per-frame update so the underlying stream gets updated (the wrapper will call UpdateMusicStream internally if needed).
- Call unload* when done to avoid leaks.
```

### Utils

#### Window & Monitor Management

All window and monitor functions are called on the `renderer` object (e.g., `renderer.ShowCursor()`).

##### Window State Queries

```js
// Check window state without modification
renderer.isWindowReady()         // => boolean - window initialized successfully
renderer.isWindowFullscreen()    // => boolean - window in fullscreen mode
renderer.isWindowHidden()        // => boolean - window currently hidden
renderer.isWindowMinimized()     // => boolean - window minimized
renderer.isWindowMaximized()     // => boolean - window maximized
renderer.isWindowFocused()       // => boolean - window has focus
renderer.isWindowResized()       // => boolean - window resized last frame
renderer.isWindowState(flag)     // => boolean - check if specific flag is set (pass WindowFlags)
```

##### Window Control

```js
// Control window state
renderer.closeWindow()                  // close window and unload OpenGL context
renderer.setWindowFocused()             // set window as focused
renderer.toggleFullscreen()             // toggle between fullscreen/windowed
renderer.toggleBorderlessWindowed()     // toggle borderless windowed mode
renderer.maximizeWindow()               // maximize window (if resizable)
renderer.minimizeWindow()               // minimize window (if resizable)
renderer.restoreWindow()                // restore from minimized/maximized
renderer.clearWindowState(flags)        // clear specific window state flags
```

##### Window Configuration

```js
// Modify window properties
renderer.setWindowTitle(title)          // set window title (string)
renderer.setWindowPosition(x, y)        // set window position on screen (number, number)
renderer.setWindowSize(width, height)   // set window dimensions (number, number)
renderer.setWindowMinSize(width, height)// set minimum window size (number, number)
renderer.setWindowMaxSize(width, height)// set maximum window size (number, number)
renderer.setWindowMonitor(monitor)      // set monitor for current window (number - monitor index)
renderer.setWindowOpacity(opacity)      // set window opacity (number 0.0-1.0)
```

##### Window Icons

```js
// Set window icon(s) - accepts file paths and loads via stb_image
renderer.setWindowIcon(imagePath)       // set single icon from image file (string path)
renderer.setWindowIcons([paths...])     // set multiple icons from file paths (array of strings)
// Note: images are loaded with stb_image, supports PNG, JPG, BMP, etc.
```

##### Screen & Render Dimensions

```js
// Get window/screen dimensions
renderer.getScreenWidth()               // => number - current screen width in pixels
renderer.getScreenHeight()              // => number - current screen height in pixels
renderer.getRenderWidth()               // => number - render width (considers HiDPI)
renderer.getRenderHeight()              // => number - render height (considers HiDPI)
renderer.getWindowPosition()            // => {x: number, y: number} - window position on monitor
renderer.getWindowScaleDPI()            // => {x: number, y: number} - DPI scale factors
renderer.getWindowHandle()              // => number - native window handle (as pointer value)
```

##### Monitor Information

```js
// Query monitor configuration
renderer.getMonitorCount()              // => number - count of connected monitors
renderer.getCurrentMonitor()            // => number - index of monitor with window
renderer.getMonitorName(monitor)        // => string - human-readable monitor name
renderer.getMonitorPosition(monitor)    // => {x: number, y: number} - monitor position
renderer.getMonitorWidth(monitor)       // => number - monitor width (current video mode)
renderer.getMonitorHeight(monitor)      // => number - monitor height (current video mode)
renderer.getMonitorPhysicalWidth(monitor)  // => number - physical width in millimeters
renderer.getMonitorPhysicalHeight(monitor) // => number - physical height in millimeters
renderer.getMonitorRefreshRate(monitor) // => number - monitor refresh rate in Hz
```

##### Clipboard

```js
// Interact with system clipboard
renderer.setClipboardText(text)         // set clipboard content (string)
renderer.getClipboardText()             // => string - get clipboard text content
renderer.getClipboardImage()            // => {width, height, format, data: Uint8Array}
                                        // get clipboard image (if available)
```

##### Event Waiting

```js
// Control event polling behavior
renderer.enableEventWaiting()           // enable waiting for events on EndDrawing()
                                        // (no automatic polling, reduces CPU usage)
renderer.disableEventWaiting()          // disable event waiting, use automatic polling
```

#### Cursor Control

All cursor functions are called on the `renderer` object (e.g., `renderer.ShowCursor()`).

```js
// Cursor visibility
renderer.showCursor()                   // show the mouse cursor
renderer.hideCursor()                   // hide the mouse cursor
renderer.isCursorHidden()               // => boolean - cursor visibility status

// Cursor locking/focusing
renderer.enableCursor()                 // unlock cursor (can move freely)
renderer.disableCursor()                // lock cursor to window
renderer.isCursorOnScreen()             // => boolean - check if cursor is on window
```

#### Utilities

```js
import {DirtyRegionTracker, ColorTheory, PerformanceMonitor, normalizeRGBA} from "tessera.js"

// DirtyRegionTracker
// constructor(canvas: PixelBuffer)
new DirtyRegionTracker(canvas)
tracker.reset()                       // reset internal bounds
tracker.markRect(x, y, w, h)         // mark changed rectangle (fast, integer ops)
tracker.mark(x, y)                   // mark single pixel (alias)
tracker.flush() -> {minX, minY, regionWidth, regionHeight} | null
  // extracts region, calls canvas.renderer.updateBufferData(...), sets canvas.needsUpload = true
  // returns null if nothing to flush



// ColorTheory (pure functions)
ColorTheory.RGBtoHSV(r, g, b) -> {h, s, v}    // s,v in 0..100
ColorTheory.HSVtoRGB(h, s, v) -> {r, g, b}    // r,g,b 0..255
ColorTheory.complementary({r,g,b}) -> {r,g,b}
ColorTheory.analogous({r,g,b}, spread = 30) -> [{r,g,b}, {r,g,b}, {r,g,b}]

// PerformanceMonitor
// constructor()
new PerformanceMonitor()
pm.start(name)                       // begin timing a named metric
pm.end(name)                         // end timing, accumulate stats
pm.recordFrameTime(startTime)        // push frame time sample
pm.logMetrics()                      // console.log aggregated metrics

// normalizeRGBA
// normalizeRGBA(r, g, b, a?) -> { r, g, b, a? }
// r,g,b returned in 0.0..1.0. If 'a' provided, returned as a/255.
normalizeRGBA(r, g, b, a)
```

## License

This project is licensed under the **AGPL-3.0-or-later License** - see the
`LICENSE` file for details.

SPDX-License-Identifier: AGPL-3.0-or-later
