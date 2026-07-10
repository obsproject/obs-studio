# obs-browser

obs-browser introduces a cross-platform Browser Source, powered by CEF ([Chromium Embedded Framework](https://bitbucket.org/chromiumembedded/cef/src/master/README.md)), to OBS Studio. A Browser Source allows the user to integrate web-based overlays into their scenes, with complete access to modern web APIs.

Additionally, obs-browser enables Service Integration (linking third party services) and Browser Docks (webpages loaded into the interface itself) on all supported platforms, except for Wayland (Linux).

**This plugin is included by default** on official packages on Windows, macOS, the Ubuntu PPA and the official [Flatpak](https://flathub.org/apps/details/com.obsproject.Studio) (most Linux distributions).

## JS Bindings

obs-browser provides a global object that allows access to some OBS-specific functionality from JavaScript. This can be used to create an overlay that adapts dynamically to changes in OBS.

### TypeScript Type Definitions

If you're using TypeScript, type definitions for the obs-browser bindings are available through npm and yarn.

```sh
# npm
npm install --save-dev @types/obs-studio

# yarn
yarn add --dev @types/obs-studio
```

### Get Browser Plugin Version

```js
/**
 * @returns {string} OBS Browser plugin version
 */
window.obsstudio.pluginVersion
// => 2.17.0
```

### Register for event callbacks

```js
/**
 * @callback EventListener
 * @param {CustomEvent} event
 */

/**
 * @param {string} type
 * @param {EventListener} listener
 */
window.addEventListener('obsSceneChanged', function(event) {
	var t = document.createTextNode(event.detail.name)
	document.body.appendChild(t)
})
```

#### Available events

Descriptions for these events can be [found here](https://obsproject.com/docs/reference-frontend-api.html?highlight=paused#c.obs_frontend_event).

* obsSceneChanged
* obsSceneListChanged
* obsTransitionChanged
* obsTransitionListChanged
* obsSourceVisibleChanged
* obsSourceActiveChanged
* obsStreamingStarting
* obsStreamingStarted
* obsStreamingStopping
* obsStreamingStopped
* obsRecordingStarting
* obsRecordingStarted
* obsRecordingPaused
* obsRecordingUnpaused
* obsRecordingStopping
* obsRecordingStopped
* obsReplaybufferStarting
* obsReplaybufferStarted
* obsReplaybufferSaved
* obsReplaybufferStopping
* obsReplaybufferStopped
* obsVirtualcamStarted
* obsVirtualcamStopped
* obsExit
* [Any custom event emitted via obs-websocket vendor requests]


### Control OBS
#### Get webpage control permissions
Permissions required: NONE
```js
/**
 * @typedef {number} Level - The level of permissions. 0 for NONE, 1 for READ_OBS (OBS data), 2 for READ_USER (User data), 3 for BASIC, 4 for ADVANCED and 5 for ALL
 */

/**
 * @callback LevelCallback
 * @param {Level} level
 */

/**
 * @param {LevelCallback} cb - The callback that receives the current control level.
 */
window.obsstudio.getControlLevel(function (level) {
    console.log(level)
})
```

#### Get OBS output status
Permissions required: READ_OBS
```js
/**
 * @typedef {Object} Status
 * @property {boolean} recording - not affected by pause state
 * @property {boolean} recordingPaused
 * @property {boolean} streaming
 * @property {boolean} replaybuffer
 * @property {boolean} virtualcam
 */

/**
 * @callback StatusCallback
 * @param {Status} status
 */

/**
 * @param {StatusCallback} cb - The callback that receives the current output status of OBS.
 */
window.obsstudio.getStatus(function (status) {
	console.log(status)
})
```

#### Get the current scene
Permissions required: READ_USER
```js
/**
 * @typedef {Object} Scene
 * @property {string} name - name of the scene
 * @property {number} width - width of the scene
 * @property {number} height - height of the scene
 */

/**
 * @callback SceneCallback
 * @param {Scene} scene
 */

/**
 * @param {SceneCallback} cb - The callback that receives the current scene in OBS.
 */
window.obsstudio.getCurrentScene(function(scene) {
    console.log(scene)
})
```

#### Get scenes
Permissions required: READ_USER
```js
/**
 * @callback ScenesCallback
 * @param {string[]} scenes
 */

/**
 * @param {ScenesCallback} cb - The callback that receives the scenes.
 */
window.obsstudio.getScenes(function (scenes) {
    console.log(scenes)
})
```

#### Get transitions
Permissions required: READ_USER
```js
/**
 * @callback TransitionsCallback
 * @param {string[]} transitions
 */

/**
 * @param {TransitionsCallback} cb - The callback that receives the transitions.
 */
window.obsstudio.getTransitions(function (transitions) {
    console.log(transitions)
})
```

#### Get current transition
Permissions required: READ_USER
```js
/**
 * @callback TransitionCallback
 * @param {string} transition
 */

/**
 * @param {TransitionCallback} cb - The callback that receives the transition currently set.
 */
window.obsstudio.getCurrentTransition(function (transition) {
    console.log(transition)
})
```

#### Save the Replay Buffer
Permissions required: BASIC
```js
/**
 * Does not accept any parameters and does not return anything
 */
window.obsstudio.saveReplayBuffer()
```

#### Start the Replay Buffer
Permissions required: ADVANCED
```js
/**
 * Does not accept any parameters and does not return anything
 */
window.obsstudio.startReplayBuffer()
```

#### Stop the Replay Buffer
Permissions required: ADVANCED
```js
/**
 * Does not accept any parameters and does not return anything
 */
window.obsstudio.stopReplayBuffer()
```

#### Change scene
Permissions required: ADVANCED
```js
/**
 * @param {string} name - Name of the scene
 */
window.obsstudio.setCurrentScene(name)
```

#### Set the current transition
Permissions required: ADVANCED
```js
/**
 * @param {string} name - Name of the transition
 */
window.obsstudio.setCurrentTransition(name)
```

#### Start streaming
Permissions required: ALL
```js
/**
 * Does not accept any parameters and does not return anything
 */
window.obsstudio.startStreaming()
```

#### Stop streaming
Permissions required: ALL
```js
/**
 * Does not accept any parameters and does not return anything
 */
window.obsstudio.stopStreaming()
```

#### Start recording
Permissions required: ALL
```js
/**
 * Does not accept any parameters and does not return anything
 */
window.obsstudio.startRecording()
```

#### Stop recording
Permissions required: ALL
```js
/**
 * Does not accept any parameters and does not return anything
 */
window.obsstudio.stopRecording()
```

#### Pause recording
Permissions required: ALL
```js
/**
 * Does not accept any parameters and does not return anything
 */
window.obsstudio.pauseRecording()
```

#### Unpause recording
Permissions required: ALL
```js
/**
 * Does not accept any parameters and does not return anything
 */
window.obsstudio.unpauseRecording()
```

#### Start the Virtual Camera
Permissions required: ALL
```js
/**
 * Does not accept any parameters and does not return anything
 */
window.obsstudio.startVirtualcam()
```

#### Stop the Virtual Camera
Permissions required: ALL
```js
/**
 * Does not accept any parameters and does not return anything
 */
window.obsstudio.stopVirtualcam()
```


### Register for visibility callbacks

**This method is legacy. Register an event listener instead.**

```js
/**
 * onVisibilityChange gets callbacks when the visibility of the browser source changes in OBS
 *
 * @deprecated
 * @see obsSourceVisibleChanged
 * @param {boolean} visibility - True -> visible, False -> hidden
 */
window.obsstudio.onVisibilityChange = function(visibility) {

};
```

### Register for active/inactive callbacks

**This method is legacy. Register an event listener instead.**

```js
/**
 * onActiveChange gets callbacks when the active/inactive state of the browser source changes in OBS
 *
 * @deprecated
 * @see obsSourceActiveChanged
 * @param {bool} True -> active, False -> inactive
 */
window.obsstudio.onActiveChange = function(active) {

};
```

### obs-websocket Vendor
obs-browser includes integration with obs-websocket's Vendor requests. The vendor name to use is `obs-browser`, and available requests are:

- `emit_event` - Takes `event_name` and ?`event_data` parameters. Emits a custom event to all browser sources. To subscribe to events, see [here](#register-for-event-callbacks)
  - See [#340](https://github.com/obsproject/obs-browser/pull/340) for example usage.

There are no available vendor events at this time.

## Building

OBS Browser cannot be built standalone. It is built as part of OBS Studio.

By following the instructions, this will enable Browser Source & Custom Browser Docks on all three platforms. Both `BUILD_BROWSER` and `CEF_ROOT_DIR` are required.

### On Windows

Follow the [build instructions](https://obsproject.com/wiki/Install-Instructions#windows-build-directions) and be sure to download the **CEF Wrapper** and set `CEF_ROOT_DIR` in CMake to point to the extracted wrapper.

### On macOS

Use the [macOS Full Build Script](https://obsproject.com/wiki/Install-Instructions#macos-build-directions). This will automatically download & enable OBS Browser.

### On Linux

Follow the [build instructions](https://obsproject.com/wiki/Install-Instructions#linux-build-directions) and choose the "If building with browser source" option. This includes steps to download/extract the CEF Wrapper, and set the required CMake variables.
