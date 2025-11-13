const { Renderer, FULLSCREEN, RESIZABLE } = require('./build/Release/renderer.node');
const { InputMap, InputBuffer } = require('./input_helpers.js');
const { utils, Texture, PixelCanvas } = require("./renderer_helpers.js")



const renderer = new Renderer();
// Initialize
if (!renderer.initialize(800, 600, "Renderer")) {
    console.error("Failed to initialize renderer");
    process.exit(1);
}
renderer.setWindowState(RESIZABLE)
const util = new utils(renderer);


console.log(renderer.width, renderer.height)

const noise = util.generateProceduralNoise(renderer.width, renderer.height)
/**
 * @type {Texture}
 */
let texture_;
if (noise) {
    texture_ = new Texture(renderer, noise.buffer.bufferId, noise.width, noise.height)
}

// console.log(noise.buffer.getData())


const canvas = new PixelCanvas(renderer, 400, 300);
// a red square in the center
for (let x = 150; x < 250; x++) {
    for (let y = 100; y < 200; y++) {
        canvas.setPixel(x, y, 255, 0, 0, 255);
    }
}

// a green diagonal line
for (let i = 0; i < 300; i++) {
    canvas.setPixel(i, i, 0, 255, 0, 255);
}

const inputMap = new InputMap(renderer.input);
inputMap.mapAction('move_left', ['A', 'ArrowLeft']);
inputMap.mapAction('move_right', ['D', 'ArrowRight']);
inputMap.mapAction('move_up', ['W', 'ArrowUp']);
inputMap.mapAction('move_down', ['S', 'ArrowDown']);
inputMap.mapAction('jump', ['Space']);
inputMap.mapAction('shoot', ['Enter']);
renderer.targetFPS = 60;
function Promisified() {
    return new Promise((resolve, reject) => {
        setTimeout(() => {
            resolve("hello I am back ")
        }, 1000);
    })
}
// Main loop
Promisified().then((d) => console.log(d))
// while (!renderer.WindowShouldClose) { 

//     renderer.beginFrame();

//     // Clear screen
//     renderer.clear({ r: 0.2, g: 0.3, b: 0.4, a: 1.0 });

//     // Draw shapes
//     renderer.drawRectangle({ x: 100, y: 100 }, { x: 200, y: 150 }, { r: 1, g: 0, b: 0, a: 1 });
//     renderer.drawCircle({ x: 400, y: 300 }, 50, { r: 0, g: 1, b: 0, a: 1 });
//     renderer.drawText("Hello Standalone Renderer!", { x: 200, y: 50 }, 20, { r: 1, g: 1, b: 1, a: 1 });

//     renderer.endFrame();
// }

// Cleanup
// renderer.shutdown();
let rectx = 100;
let recty = 100
renderer.onRender(() => {
    //     texture_.update() // runs only if buffer is dirty
    // texture_.draw(0, 0);
    canvas.update();
    canvas.draw(0, 0);
    // renderer.drawRectangle({ x: rectx, y: recty }, { x: 200, y: 150 }, { r: 1, g: 0, b: 0, a: 1 });
    // renderer.drawCircle({ x: 400, y: 300 }, 50, { r: 0, g: 1, b: 0, a: 1 });
    // renderer.drawText("Hello Standalone Renderer!", { x: 200, y: 50 }, 20, { r: 1, g: 1, b: 1, a: 1 });


})

function Loop() {
    renderer.input.GetInput()
    if (inputMap.isActionActive('move_left')) {
        console.log("move left")
        rectx -= 10;
    }
    if (inputMap.isActionActive('move_right')) {
        console.log("move right")
        rectx += 10;
    }

    if (renderer.step()) {
        setImmediate(Loop);
    } else {
        console.log('loop ended');
        renderer.shutdown();
    }
}

Loop();


process.on('SIGINT', () => {
    console.log('\nShutting down gracefully...');
    renderer.shutdown();
    process.exit(0);
});