OBS Studio <https://obsproject.com>
===================================

.. image:: https://dev.azure.com/obsjim/obsjim/_apis/build/status/obsproject.obs-studio?branchName=master
   :alt: OBS Studio Build Status - Azure Pipelines
   :target: https://dev.azure.com/obsjim/obsjim/_build/latest?definitionId=1&branchName=master

.. image:: https://d322cqt584bo4o.cloudfront.net/obs-studio/localized.svg
   :alt: OBS Studio Translation Project Progress
   :target: https://crowdin.com/project/obs-studio

.. image:: https://discordapp.com/api/guilds/348973006581923840/widget.png?style=shield
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

 - Bug Tracker: https://obsproject.com/mantis/

   (Note: The bug tracker is linked to forum accounts.  To use the bug
   tracker, log in to a forum account)

Building from source
--------------------

 Clone the repository and submodules
  .. code-block:: text

      git clone --recursive https://github.com/obsproject/obs-studio.git

**Windows:**

  Install the following prerequisites: 

  1. `Qt 5.14.1 <https://www.qt.io/download-qt-installer?hsCtaTracking=99d9dd4f-5681-48d2-b096-470725510d34%7C074ddad0-fdef-4e53-8aa8-5e8a876d6ab4>`_

  2. `Cmake <https://cmake.org/>`_

  3. Pre-built windows dependencies for VS2017  https://obsproject.com/downloads/dependencies2017.zip
  4. `Visual Studio 2019 <https://visualstudio.microsoft.com/vs/older-downloads/>`_
  5. `LLVM <https://releases.llvm.org/>`_
  6. `Embedded chrome browser library <http://opensource.spotify.com/cefbuilds/index.html>`_  version 08/29/2018 - CEF 3.3440.1806.g65046b7 / Chromium 68.0.3440.106 
  
  After installing the `prerequisites <https://github.com/obsproject/obs-studio/wiki/Install-Instructions>`_ .Create the following environment variables:

  #. **QTDIR** - Path pointing Qt 5.14.1 msvc2017_64 folder

  #. **obsInstallerTempDir** - Empty directory path

  #. **DepsPath** - Path to pre-built windows dependencies win64/include

  #. **LIBCAFFEINE_DIR** - Path to `prebuilt libcaffeine <https://github.com/caffeinetv/libcaffeine/releases>`_  folder

  #. **CEF_ROOT_DIR** - Path to the embedded chrome browser library 

  Run the automated build script: ``build.bat [OPTION]``   

  .. csv-table:: 
   :header: "Option", "Usage"
   :widths: 20, 80

   "*-help*", "To display the supported options."
   "*-check*", "To verify project prerequisites are set."
   "*-build*", "To build 64 bit version of obs-studio."
   "*-package*", "To build package."
  

**Mac:**

  Install following prerequisites: 

  - Homebrew
  - `Qt <https://www.qt.io/download-qt-installer?hsCtaTracking=99d9dd4f-5681-48d2-b096-470725510d34%7C074ddad0-fdef-4e53-8aa8-5e8a876d6ab4>`_

  Build steps:

  Open Terminal 

  1. Install FFmpeg

     .. code-block:: text

      brew tap homebrew-ffmpeg/ffmpeg

      brew install homebrew-ffmpeg/ffmpeg/ffmpeg --with-srt

  2. Set environment variable for Qt

     .. code-block:: text

      export QTDIR="path/to/Qt"

      export DYLID_FRAMEWORK_PATH="path/to/Qt/5.14.1/clang_64/lib"

  3. Install cmake
 
     .. code-block:: text

      brew install cmake

  4. Change directory obs-studio directory 

     .. code-block:: text

      mkdir build

      cd build 

      cmake .. & make

  5. After it built successfully then run the app 
     
     .. code-block:: text

      cd rundir/RelWithDebInfo/bin

      ./obs

  
Contributing
------------

 - If you would like to help fund or sponsor the project, you can do so
   via `Patreon <https://www.patreon.com/obsproject>`_, `OpenCollective
   <https://opencollective.com/obsproject>`_, or `PayPal
   <https://www.paypal.me/obsproject>`_.  See our `contribute page
   <https://obsproject.com/contribute>`_ for more information.

 - If you wish to contribute code to the project, please make sure to
   read the coding and commit guidelines:
   https://github.com/obsproject/obs-studio/blob/master/CONTRIBUTING.rst

 - Developer/API documentation can be found here:
   https://obsproject.com/docs

 - If you wish to contribute translations, do not submit pull requests.
   Instead, please use Crowdin.  For more information read this thread:
   https://obsproject.com/forum/threads/how-to-contribute-translations-for-obs.16327/

 - Other ways to contribute are by helping people out with support on
   our forums or in our community chat.  Please limit support to topics
   you fully understand -- bad advice is worse than no advice.  When it
   comes to something that you don't fully know or understand, please
   defer to the official help or official channels.
