Neural Studio - Final Repository Structure
Top-Level Organization
Neural-Studio/
├── test/                    # All test suites (unit, integration, e2e)
├── build/                   # Build artifacts and CMake configuration (Ubuntu/Pop!_OS only)
├── deps/                    # Third-party dependencies
├── docs/                    # Documentation
├── api/                     # Public API definitions
├── locale/                  # Localization/i18n files (if top-level, otherwise in ui/shared_ui/)
├── core/                    # Core engine and backend systems
├── ui/                      # User interface layer
├── drivers/                 # Hardware-specific functionality bridges
├── plugins/                 # Functionality plugins (not hardware-specific)
├── addons/                  # WASM-based user/company integrations
├── profiles/                # Device/system configuration profiles
├── models/                  # AI/ML model files
└── assets/                  # Production media and resources
Detailed Structure
/test/
test/
├── unit/                    # Unit tests
├── integration/             # Integration tests
└── e2e/                     # End-to-end tests
/build/
build/
├── cmake/                   # CMake build system logic
├── config/                  # Build configuration files
└── out/                     # Compiled output (gitignored)
Note: Ubuntu/Pop!_OS only. No Windows/macOS platform-specific code.

/deps/
deps/
├── blake2/                  # Cryptographic hashing
├── glad/                    # OpenGL loader
├── json11/                  # JSON parsing
├── libcaption/              # Closed captioning (CEA-608/708)
├── libdshowcapture/         # DirectShow capture (legacy, may remove)
└── w32-pthreads/            # Windows pthreads (legacy, may remove)
/docs/
docs/
├── architecture/            # Architecture decision records
├── api/                     # API documentation
├── user-guide/              # End-user documentation
└── licenses/                # Third-party licenses
/api/
api/
├── frontend/                # Frontend API for plugins
├── core/                    # Core engine API
└── scripting/               # Scripting API (Lua/Python)
/core/
core/
├── include/                 # Public headers
├── lib/                     # Core libraries
│   ├── qt/                  # Qt integration utilities
│   ├── obs/                 # OBS core library
│   ├── vr/                  # VR-specific core library
│   ├── opengl/              # OpenGL rendering backend (legacy for plugins/addons)
│   └── platform/            # Ubuntu/Pop!_OS platform-specific code
├── src/                     # Core implementation
│   ├── audio/               # Audio engine
│   ├── video/               # Video engine
│   ├── rendering/           # Rendering systems
│   ├── compositor/          # 3D-first layer mixer/compositor (supports 2D)
│   ├── scene-graph/         # Semantic scene graph
│   └── ipc/                 # Inter-process communication
├── utilities/               # Shared utilities
│   ├── ipc/                 # IPC utilities
│   ├── media-playback/      # Media playback engine
│   ├── file-updater/        # File update utilities
│   └── happy-eyeballs/      # Network connection optimization
├── settings/                # Backend configuration system
├── registry/                # Component registry
├── keys/                    # Cryptographic keys and certificates
├── protocols/               # Protocol definitions
│   └── vr/                  # VR protocol (VRProtocol.h)
├── scripting/               # Lua/Python scripting engine
├── importers/               # Import from other software (XSplit, Streamlabs, etc.)
├── oauth/                   # OAuth/authentication (Twitch, YouTube, etc.)
├── updater/                 # Auto-update system
├── ai/                      # AI integration layer
├── ml/                      # Machine learning components
├── dl/                      # Deep learning components
├── encoding/                # Video/audio encoding
└── decoding/                # Video/audio decoding
/ui/
ui/
├── shared_ui/               # Shared UI components
│   ├── locale/              # Localization files (all languages)
│   ├── forms/               # Shared UI form definitions (.ui files)
│   ├── widgets/             # Reusable widgets (VRButton, VRSlider, VRFilePicker)
│   ├── themes/              # Theme manager
│   └── components/          # Common UI components
├── blueprint/               # Node graph editor
│   ├── nodes/               # Node implementations
│   │   ├── VideoInputNode/
│   │   │   ├── VideoInputNode.cpp
│   │   │   ├── VideoInputNode.h
│   │   │   ├── VideoInputNode.qml
│   │   │   └── VideoInputNode.ui
│   │   ├── AudioInputNode/
│   │   ├── FilterNode/
│   │   ├── EffectNode/
│   │   ├── TransitionNode/
│   │   └── LLMTranscriptionNode/
│   ├── widgets/             # Graph UI widgets
│   │   ├── ConnectionItem/
│   │   ├── NodeItem/
│   │   └── PortItem/
│   ├── core/                # Graph core logic
│   │   ├── NodeGraph.cpp/.h
│   │   ├── NodeModel.cpp/.h
│   │   ├── NodeRegistry.cpp/.h
│   │   ├── ConnectionModel.cpp/.h
│   │   └── GraphSerializer.cpp/.h
│   ├── panels/              # Blueprint panels
│   │   └── PropertiesPanel/
│   │       ├── PropertiesPanel.cpp/.h
│   │       └── PropertiesPanel.qml
│   └── viewmodels/          # MVVM ViewModels
│       ├── MixerViewModel.cpp/.h
│       ├── PropertiesViewModel.cpp/.h
│       └── SettingsViewModel.cpp/.h
├── activeui/                # Active monitoring interface
│   ├── monitors/            # Monitor widgets
│   │   ├── VideoModule/
│   │   ├── AudioMixer/
│   │   ├── CamModule/
│   │   ├── MonitorGrid/
│   │   └── MonitorWidget/
│   └── controls/            # Control widgets
│       ├── SceneButtons/
│       ├── ExtraModules/
│       ├── MoreModules/
│       └── ThreeDModule/
├── docks/                   # Dockable panels
│   ├── BrowserDock/
│   ├── YouTubeAppDock/
│   └── YouTubeChatDock/
├── global/                  # Application shell
│   ├── menus/               # Application menus
│   ├── wizards/             # Setup wizards (AutoConfig, etc.)
│   │   ├── AutoConfigWizard/
│   │   └── forms/
│   ├── dialogs/             # Global dialogs
│   │   ├── AboutDialog/
│   │   ├── SettingsDialog/
│   │   ├── FiltersDialog/
│   │   ├── TransformDialog/
│   │   └── PermissionsDialog/
│   └── app/                 # Main application logic
│       ├── OBSApp.cpp/.hpp
│       └── main.cpp
└── settings/                # Settings UI layer
    ├── GeneralSettings/
    ├── AudioSettings/
    ├── VideoSettings/
    ├── OutputSettings/
    ├── StreamSettings/
    ├── AccessibilitySettings/
    └── AppearanceSettings/
/drivers/
Hardware-specific functionality bridges

drivers/
├── audio/                   # Audio hardware drivers
│   ├── alsa/                # ALSA (Linux)
│   ├── jack/                # JACK Audio
│   ├── pipewire/            # PipeWire
│   └── pulseaudio/          # PulseAudio
├── video/                   # Video capture hardware
│   ├── v4l2/                # Video4Linux2
│   └── uvc/                 # USB Video Class
├── midi/                    # MIDI device drivers
├── input/                   # Input device drivers
├── output/                  # Output device drivers
├── network/                 # Network protocols
│   ├── http/                # HTTP streaming
│   ├── websocket/           # WebSocket
│   ├── webrtc/              # WebRTC
│   ├── webtransport/        # WebTransport
│   ├── rtmp/                # RTMP
│   ├── srt/                 # SRT (Secure Reliable Transport)
│   └── whip/                # WHIP
├── storage/                 # Storage drivers
├── accelerators/            # GPU accelerators
│   ├── cuda/                # NVIDIA CUDA
│   ├── nvenc/               # NVIDIA NVENC
│   ├── vulkan/              # Vulkan
│   └── vaapi/               # VA-API (Linux)
├── aja/                     # AJA hardware
│   ├── aja-output/          # Backend
│   └── aja-output-ui/       # UI components
└── decklink/                # Blackmagic DeckLink
    ├── decklink-output/     # Backend
    ├── decklink-captions/   # Captions support
    └── decklink-output-ui/  # UI components
/plugins/
Functionality plugins (not hardware-specific) - C++

plugins/
├── image-source/            # Image sources
├── obs-ffmpeg/              # FFmpeg integration
├── obs-filters/             # Video/audio filters
├── obs-outputs/             # Output formats
├── obs-transitions/         # Scene transitions
├── obs-text/                # Text sources
├── obs-websocket/           # WebSocket control API
├── obs-browser/             # Browser source (CEF)
├── obs-vst/                 # VST audio plugin support
├── frontend-tools/          # Frontend tools
│   ├── auto-scene-switcher/
│   ├── captions/
│   ├── output-timer/
│   └── scripts/
└── rtmp-services/           # RTMP service integrations
/addons/
WASM-based user/company integrations

addons/
├── examples/                # Example addons
├── community/               # Community-contributed addons
└── marketplace/             # Marketplace addons
/profiles/
profiles/
├── audio/                   # Audio device profiles
├── video/                   # Video device profiles
├── midi/                    # MIDI device profiles
├── input/                   # Input device profiles
├── output/                  # Output device profiles
├── network/                 # Network profiles
├── storage/                 # Storage profiles
├── accelerators/            # GPU accelerator profiles
├── aja/                     # AJA hardware profiles
├── decklink/                # DeckLink hardware profiles
└── vr-headsets/             # VR headset profiles
    ├── quest3.json
    ├── vive_pro_2.json
    ├── index.json
    └── pico4.json
/models/
models/
├── audio/                   # Audio AI models
│   ├── noise-suppression/
│   └── speech-enhancement/
├── video/                   # Video AI models
│   ├── super-resolution/
│   ├── segmentation/
│   └── style-transfer/
└── llm/                     # Large language models
    └── transcription/
/assets/
assets/
├── audio/                   # Audio assets
│   ├── sfx/
│   └── music/
├── video/                   # Video assets
│   └── overlays/
├── 3d/                      # 3D models and scenes
│   ├── models/
│   └── scenes/
├── effects/                 # Visual effects
│   └── shaders/
├── fonts/                   # Font files
│   ├── OpenSans-Regular.ttf
│   ├── OpenSans-Bold.ttf
│   └── OpenSans-Italic.ttf
├── icons/                   # Icon sets
│   ├── settings/
│   ├── sources/
│   └── media/
├── shaders/                 # Shader programs
│   ├── glsl/
│   └── hlsl/
├── textures/                # Texture assets
├── themes/                  # UI themes
│   ├── Dark/
│   ├── Light/
│   ├── Acri/
│   ├── Rachni/
│   └── Yami/
└── licenses/                # Asset licenses
Key Design Principles
Co-location: Forms, QML, and UI definitions live with the component they serve
Semantic Organization: Each top-level folder has clear semantic subfolders
Hardware vs. Functionality:
Drivers = Hardware-specific bridges
Plugins = Functionality extensions (C++)
Addons = Safe WASM integrations
Ubuntu/Pop!_OS Only: No Windows/macOS platform code
3D-First Design: Compositor supports 2D but prioritizes 3D workflows
OpenGL Legacy: Kept for backward compatibility with plugins/addons
Migration Notes
Platform-specific code → core/lib/platform/ (Ubuntu only)
Legacy Windows dependencies (w32-pthreads, libdshowcapture) → Mark for removal
Localization files → ui/shared_ui/locale/
Cryptographic keys → core/keys/
All utilities → core/utilities/ (don't over-organize)