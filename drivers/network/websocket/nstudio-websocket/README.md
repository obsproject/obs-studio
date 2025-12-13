<!-- markdownlint-disable no-inline-html -->

# obs-websocket

<p align="center">
  <img src="/.github/images/obsws_logo.png" width=150 align="center">
</p>

WebSocket API for OBS Studio.

[![Discord](https://img.shields.io/discord/715691013825364120.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/WBaSQ3A)
[![Financial Contributors on Open Collective](https://opencollective.com/obs-websocket-dev/all/badge.svg?label=financial+contributors)](https://opencollective.com/obs-websocket-dev)

## Downloads

**obs-websocket is now included by default with OBS Studio 28.0.0 and above. As such, there should be no need to download obs-websocket if you have OBS Studio > 28.0.0.**

Binaries **for OBS Studio < 28.0.0** on Windows, MacOS, and Linux are available in the [Releases](https://github.com/obsproject/obs-websocket/releases) section.

## Using obs-websocket

It is **highly recommended** to keep obs-websocket protected with a password against unauthorized control. obs-websocket generates a password for you automatically when you load it for the first time. To change this, open the "obs-websocket Settings" dialog under OBS' "Tools" menu. In the settings dialog, you can enable or disable authentication and set a password for it.

(Psst. You can use `--websocket_port`(value), `--websocket_password`(value), `--websocket_debug`(flag) and `--websocket_ipv4_only`(flag) on the command line to override the configured values.)

### Possible use cases

- Remote control OBS from a phone or tablet on the same local network
- Change your stream overlay/graphics based on the current scene
- Automate scene switching with a third-party program (e.g. : auto-pilot, foot pedal, ...)

### Client software

- [Macro Deck](https://macrodeck.org/)
- [Touch Portal](https://www.touch-portal.com/)
- [Twitchat](https://twitchat.fr/)
- [OBS-web](https://github.com/Niek/obs-web) - hosted client at [obs-web.niek.tv/](http://obs-web.niek.tv/)
- [Streamer.bot](https://streamer.bot/)
- [Deckboard](https://deckboard.app/)
- [OBS Blade](https://github.com/Kounex/obs_blade)
- [Aitum](https://aitum.tv/)
- [Kruiz Control](https://github.com/Kruiser8/Kruiz-Control)
- [Bitfocus Companion Module](https://bitfocus.io/companion/)
- [MacroGraph](https://github.com/Brendonovich/macrograph) - hosted client [here](https://macrograph.brendonovich.dev/)
- [MATRIC](https://matricapp.com/)

### Client libraries (for developers)

Here's a list of available language APIs for obs-websocket:

- Python 3.7+ (Asyncio): [simpleobsws](https://github.com/IRLToolkit/simpleobsws/tree/master) by IRLToolkit
- Python 3.10+ (Non-Asyncio): [obsws-python](https://pypi.org/project/obsws-python) by aatikturk and onyx-and-iris
- Rust: [obws](https://github.com/dnaka91/obws) by dnaka91
- Godot 4.0.x: [obs-websocket-gd](https://github.com/you-win/obs-websocket-gd) by you-win
- Javascript (Node and web): [obs-websocket-js](https://github.com/obs-websocket-community-projects/obs-websocket-js) by OBS Websocket Community
  - C (uses obs-websocket-js): [v8-libwebsocket-obs-websocket](https://github.com/dgatwood/v8-libwebsocket-obs-websocket)
- Go: [goobs](https://github.com/andreykaipov/goobs) by andreykaipov
- Dart/Flutter (can target all supported platforms): [obs_websocket](https://github.com/faithoflifedev/obs_websocket) by faithoflifedev
- Java: [obs-websocket-java](https://github.com/obs-websocket-community-projects/obs-websocket-java) by OBS Websocket Community

The 5.x server is a typical WebSocket server running by default on port 4455 (the port number can be changed in the Settings dialog under `Tools`).
The protocol we use is documented in [PROTOCOL.md](docs/generated/protocol.md).

We'd like to know what you're building with obs-websocket! If you do something in this fashion, feel free to drop a message in `#project-showoff` in the [discord server!](https://discord.gg/WBaSQ3A)

## Contributors

### Code Contributors

This project exists thanks to [all the people](https://github.com/obsproject/obs-websocket/graphs/contributors) who contribute. [Contribute Code](https://github.com/obsproject/obs-websocket/wiki/Contributing-Guidelines).

### Financial Contributors

Become a financial contributor and help us sustain our community. [Contribute Financially](https://opencollective.com/obs-websocket-dev/contribute)

#### Individuals

<a href="https://opencollective.com/obs-websocket-dev"><img src="https://opencollective.com/obs-websocket-dev/individuals.svg?width=890"></a>

#### Organizations

Support this project with your organization. Your logo will show up here with a link to your website. [Contribute](https://opencollective.com/obs-websocket-dev/contribute)

<a href="https://opencollective.com/obs-websocket-dev/organization/0/website"><img src="https://opencollective.com/obs-websocket-dev/organization/0/avatar.svg"></a>
<a href="https://opencollective.com/obs-websocket-dev/organization/1/website"><img src="https://opencollective.com/obs-websocket-dev/organization/1/avatar.svg"></a>
<a href="https://opencollective.com/obs-websocket-dev/organization/2/website"><img src="https://opencollective.com/obs-websocket-dev/organization/2/avatar.svg"></a>
<a href="https://opencollective.com/obs-websocket-dev/organization/3/website"><img src="https://opencollective.com/obs-websocket-dev/organization/3/avatar.svg"></a>
<a href="https://opencollective.com/obs-websocket-dev/organization/4/website"><img src="https://opencollective.com/obs-websocket-dev/organization/4/avatar.svg"></a>
<a href="https://opencollective.com/obs-websocket-dev/organization/5/website"><img src="https://opencollective.com/obs-websocket-dev/organization/5/avatar.svg"></a>
<a href="https://opencollective.com/obs-websocket-dev/organization/6/website"><img src="https://opencollective.com/obs-websocket-dev/organization/6/avatar.svg"></a>
<a href="https://opencollective.com/obs-websocket-dev/organization/7/website"><img src="https://opencollective.com/obs-websocket-dev/organization/7/avatar.svg"></a>
<a href="https://opencollective.com/obs-websocket-dev/organization/8/website"><img src="https://opencollective.com/obs-websocket-dev/organization/8/avatar.svg"></a>
<a href="https://opencollective.com/obs-websocket-dev/organization/9/website"><img src="https://opencollective.com/obs-websocket-dev/organization/9/avatar.svg"></a>
