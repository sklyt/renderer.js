const fs = require('fs');
const path = require('path');
const { Renderer, FULLSCREEN, RESIZABLE } = require('./build/Release/renderer.node');
const { InputMap } = require('./input_helpers.js');

const renderer = new Renderer();

if (!renderer.initialize(800, 600, "Renderer Demo")) {
  console.error("Failed to initialize renderer");
  process.exit(1);
}

console.log("Init shared",renderer.initSharedBuffers)

renderer.setWindowState(RESIZABLE);
renderer.targetFPS = 60;

const inputMap = new InputMap(renderer.input);
inputMap.mapAction('move_left', ['A', 'ArrowLeft']);
inputMap.mapAction('move_right', ['D', 'ArrowRight']);
inputMap.mapAction('jump', ['Space']);

const audio = renderer.audio;

// load sound from disk and play immediately
const soundPath = "/home/sifundo/workspace/C/clem/scripts/assets/sounds/Technology/Camila Cabello  I LUV IT Feat Playboi Carti Official Music Video.ogg";
const soundHandle = audio.loadMusic(soundPath, true);
console.log('disk sound handle', soundHandle);
if (soundHandle) audio.playMusic(soundHandle);

// load a sound from memory (Buffer / TypedArray / ArrayBuffer / SharedArrayBuffer supported)
const memFilePath = path.join(__dirname, 'assets', 'click.wav'); // change to a real small file
let memSoundHandle = 0;
try {
  const buf = fs.readFileSync(memFilePath); // Node Buffer
  memSoundHandle = audio.loadSoundFromMemory('wav', buf);
  console.log('memory sound handle', memSoundHandle);
} catch (e) {
  console.warn('failed to read mem file:', e.message);
}

// load music from disk and from memory
const musicPath = path.join(__dirname, 'assets', 'background.ogg'); // change to a real file
const musicHandle = audio.loadMusic(musicPath);
console.log('disk music handle', musicHandle);

// optional: music from memory
let memMusicHandle = 0;
try {
  const musicBuf = fs.readFileSync(path.join(__dirname, 'assets', 'bg_small.ogg'));
  memMusicHandle = audio.loadMusicFromMemory('ogg', musicBuf);
  console.log('memory music handle', memMusicHandle);
} catch (e) {
  console.warn('failed to read mem music:', e.message);
}

// show SharedBuffer usage (renderer-side shared buffer API)
// createSharedBuffer returns an id for a renderer-managed SharedArrayBuffer
const bufferId = renderer.createSharedBuffer(800 * 600 * 4);
console.log('shared buffer id', bufferId);

// simple render callback
let rectx = 100;
renderer.onRender(() => {
  renderer.clear({ r: 0.1, g: 0.12, b: 0.15, a: 1 });
  renderer.drawRectangle({ x: rectx, y: 100 }, { x: 200, y: 150 }, { r: 1, g: 0, b: 0, a: 1 });
});

// main loop
function Loop() {
  renderer.input.GetInput();
  if (inputMap.isActionActive('move_left')) rectx -= 6;
  if (inputMap.isActionActive('move_right')) rectx += 6;

  if (renderer.step()) setImmediate(Loop);
  else {
    console.log('renderer loop ended');
    renderer.shutdown();
  }
}
Loop();

// chain audio operations with setTimeouts
setTimeout(() => {
  if (memSoundHandle) {
    console.log('play memory sound');
    audio.playSound(memSoundHandle);
  }
}, 1000);

setTimeout(() => {
  console.log('lower master volume to 0.5');
  audio.setMasterVolume(0.5);
}, 2000);

setTimeout(() => {
  if (soundHandle) {
    console.log('set sound volume to 0.2 (per-sound)');
    audio.setSoundVolume(soundHandle, 0.2);
  }
}, 3000);

setTimeout(() => {
  if (musicHandle) {
    console.log('play disk music');
    audio.playMusic(musicHandle);
  } else if (memMusicHandle) {
    console.log('play memory music');
    audio.playMusic(memMusicHandle);
  }
}, 3500);

setTimeout(() => {
  if (musicHandle) {
    console.log('pause music');
    audio.pauseMusic(musicHandle);
  }
}, 7000);

setTimeout(() => {
  if (musicHandle) {
    console.log('resume music');
    audio.resumeMusic(musicHandle);
  }
}, 9000);

setTimeout(() => {
  if (soundHandle) {
    console.log('is sound playing?', audio.isSoundPlaying(soundHandle));
    console.log('stop sound and unload it');
    audio.stopSound(soundHandle);
    audio.unloadSound(soundHandle);
  }
}, 11000);

setTimeout(() => {
  if (memSoundHandle) {
    console.log('stop and unload memory sound');
    audio.stopSound(memSoundHandle);
    audio.unloadSound(memSoundHandle);
  }
  if (musicHandle) {
    console.log('stop and unload music');
    audio.stopMusic(musicHandle);
    audio.unloadMusic(musicHandle);
  }
  if (memMusicHandle) {
    audio.stopMusic(memMusicHandle);
    audio.unloadMusic(memMusicHandle);
  }
}, 13000);

setTimeout(() => {
  console.log('restore master volume, then shutdown renderer');
  audio.setMasterVolume(1.0);
}, 14500);

setTimeout(() => {
  console.log('shutdown renderer and exit');
  renderer.shutdown();
  process.exit(0);
}, 15000);

// graceful SIGINT cleanup
process.on('SIGINT', () => {
  console.log('SIGINT -> shutdown');
  renderer.shutdown();
  process.exit(0);
});
