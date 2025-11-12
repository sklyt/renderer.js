class InputMap {
    constructor(input) {
        this.input = input;
        this.actions = new Map();
        this.mouseActions = new Map();
        this.callbackIds = [];
    }

    // Map an action to one or more keys
    mapAction(actionName, keys) {
        if (typeof keys === 'string') {
            keys = [keys];
        }
        this.actions.set(actionName, keys);
    }
    MapMouseAction(actionName, keys) {
        if (typeof keys === 'string') {
            keys = [keys];
        }
        this.mouseActions.set(actionName, keys)
    }

    isMouseActionActive(actionName) {
        const keys = this.mouseActions.get(actionName);
        if (!keys) return false;

        // console.log("checking mouse: ", keys)

        return keys.some(key => this.input.isMouseButtonDown(key));
    }
    isMousePressed(actionName){
                const keys = this.mouseActions.get(actionName);
        if (!keys) return false;

        // console.log("checking mouse: ", keys)

        return keys.some(key => this.input.isMouseButtonPressed(key));
    }
    // Check if an action is currently active
    isActionActive(actionName) {
        const keys = this.actions.get(actionName);
        if (!keys) return false;

        return keys.some(key => this.input.isKeyDown(key));
    }

    // Check if an action was just triggered this frame
    wasActionTriggered(actionName) {
        const keys = this.actions.get(actionName);
        if (!keys) return false;

        return keys.some(key => this.input.isKeyPressed(key));
    }

    // TODO: wrap in c++
    onAction(actionName, callback) {
        const keys = this.actions.get(actionName);
        if (!keys) {
            console.warn(`Action '${actionName}' not mapped`);
            return;
        }

        keys.forEach(key => {
            const id = this.input.onKeyDown(key, (event) => {
                callback(actionName, event);
            });
            this.callbackIds.push(id);
        });
    }

    cleanup() {
        this.callbackIds.forEach(id => {
            this.input.removeCallback(id);
        });
        this.callbackIds = [];
    }
}

class InputBuffer {
    constructor(maxSize = 60) { // 1 second at 60 FPS
        this.maxSize = maxSize;
        this.buffer = [];
    }

    recordInput(inputName, timestamp) {
        this.buffer.push({ input: inputName, timestamp });

        // Keep buffer size manageable
        if (this.buffer.length > this.maxSize) {
            this.buffer.shift();
        }
    }

    // Check for input sequences (useful for combos, cheats, etc.)
    checkSequence(sequence, timeWindow = 1000) { // 1 second window
        if (sequence.length === 0 || this.buffer.length < sequence.length) {
            return false;
        }

        const now = performance.now();
        let sequenceIndex = 0;

        // Work backwards through buffer
        for (let i = this.buffer.length - 1; i >= 0 && sequenceIndex < sequence.length; i--) {
            const record = this.buffer[i];

            // Check if this record is too old
            if (now - record.timestamp > timeWindow) {
                break;
            }

            // Check if this matches the next expected input in sequence
            if (record.input === sequence[sequence.length - 1 - sequenceIndex]) {
                sequenceIndex++;
            }
        }

        return sequenceIndex === sequence.length;
    }
}

module.exports = { InputMap, InputBuffer };