OBS Studio <https://obsproject.com>
===================================

OBS Studio WebGPU - Windows Preview
-----------------------------------

This fork adds experimental WebGPU support for Browser Sources on Windows. It
is intended for creators who need WebGPU content in an OBS Browser Source,
while preserving a stable fallback for systems where browser GPU acceleration
is unavailable.

Download and start (Windows)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Open the latest release:
   https://github.com/Loggableim/obs-studio-webgpu/releases/latest
2. Download OBS-Studio-WebGPU-Windows-x64-preview.zip.
3. Extract the ZIP completely to a folder of your choice.
4. Open bin\\64bit\\obs64.exe with a double-click.

Do not move only obs64.exe out of the extracted folder. It needs the included
CEF runtime, plugins, and data directory beside it.

Enable WebGPU for Browser Sources
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. In OBS, open Settings -> Advanced.
2. Set Browser Source WebGPU mode to Auto.
3. Restart OBS when prompted.
4. Add or refresh a Browser Source with an HTTPS WebGPU page, for example
   https://toji.github.io/webgpu-test/.

WebGPU is disabled by default for new profiles so that unsupported systems
continue to start reliably. Intel Arc adapters are recognised as WebGPU-capable
and are not sent through the legacy Intel GPU fallback.

Important
~~~~~~~~~

- This is a preview build. Test new Browser Sources before using them in a
  live production.
- Use HTTPS whenever possible. WebGPU requires a secure context.
- If a source stays blank, first refresh the Browser Source and check that
  WebGPU mode is set to Auto.

.. image:: https://github.com/obsproject/obs-studio/actions/workflows/push.yaml/badge.svg?branch=master
   :alt: OBS Studio Build Status - GitHub Actions
   :target: https://github.com/obsproject/obs-studio/actions/workflows/push.yaml?query=branch%3Amaster

.. image:: https://badges.crowdin.net/obs-studio/localized.svg
   :alt: OBS Studio Translation Project Progress
   :target: https://crowdin.com/project/obs-studio

.. image:: https://img.shields.io/discord/348973006581923840.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2
   :alt: OBS Studio Discord Server
   :target: https://obsproject.com/discord

What is OBS Studio?
-------------------

OBS Studio is software designed for capturing, compositing, encoding,
recording, and streaming video content, efficiently.

It's distributed under the GNU General Public License v2 (or any later
version) - see the accompanying COPYING file for more details.

Quick Links
-----------

- Website: https://obsproject.com

- Help/Documentation/Guides: https://github.com/obsproject/obs-studio/wiki

- Forums: https://obsproject.com/forum/

- Build Instructions: https://github.com/obsproject/obs-studio/wiki/Install-Instructions

- Developer/API Documentation: https://obsproject.com/docs

- Donating/backing/sponsoring: https://obsproject.com/contribute

- Bug Tracker: https://github.com/obsproject/obs-studio/issues

Contributing
------------

- If you would like to help fund or sponsor the project, you can do so
  via `Patreon <https://www.patreon.com/obsproject>`_, `OpenCollective
  <https://opencollective.com/obsproject>`_, or `PayPal
  <https://www.paypal.me/obsproject>`_.  See our `contribute page
  <https://obsproject.com/contribute>`_ for more information.

- If you wish to contribute code to the project, please make sure to
  read the coding and commit guidelines:
  https://github.com/obsproject/obs-studio/blob/master/CONTRIBUTING.md
  
- Code for the project follows the code style guidelines, located
  here: https://github.com/obsproject/obs-studio/blob/master/CODESTYLE.md

- Developer/API documentation can be found here:
  https://obsproject.com/docs

- If you wish to contribute translations, do not submit pull requests.
  Instead, please use Crowdin.  For more information read this page:
  https://obsproject.com/wiki/How-To-Contribute-Translations-For-OBS

- Contributors to OBS Studio and related repositories are expected to
  follow our Code of Conduct, which can be read here:
  https://github.com/obsproject/obs-studio/blob/master/COC.rst

- Other ways to contribute are by helping people out with support on
  our forums or in our community chat.  Please limit support to topics
  you fully understand -- bad advice is worse than no advice.  When it
  comes to something that you don't fully know or understand, please
  defer to the official help or official channels.


SAST Tools
----------

`PVS-Studio <https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source>`_ - static analyzer for C, C++, C#, and Java code.
