Contributing
============

Quick Links for Contributing
----------------------------

 - Compiling and building OBS Studio:
   https://github.com/jp9000/obs-studio/wiki/Install-Instructions

 - Our bug tracker (linked to forum accounts):
   https://obsproject.com/mantis/

 - Development IRC channel: #obs-dev on QuakeNet
 
 - Development forum:
   https://obsproject.com/forum/list/general-development.21/

 - To contribute language translations, do not make pull requests.
   Instead, use crowdin.  Read here for more information:
   https://obsproject.com/forum/threads/how-to-contribute-translations-for-obs.16327/

Coding Guidelines
-----------------

 - OBS Studio uses kernel normal form (linux variant), for more
   information, please read here:
   https://www.kernel.org/doc/Documentation/CodingStyle

 - Avoid trailing spaces.  To view trailing spaces before making a
   commit, use "git diff" on your changes.  If colors are enabled for
   git in the command prompt, it will show you any whitespace issues
   marked with red.

 - Tabs for indentation, spaces for alignment.  Tabs are treated as 8
   columns wide.

 - 80 columns max

Commit Guidlines
----------------

 - OBS Studio uses the 50/72 standard for commits.  50 characters max
   for the title (excluding module prefix), an empty line, and then a
   full description of the commit, wrapped to 72 columns max.  See this
   link for more information: http://chris.beams.io/posts/git-commit/

 - Make sure commit titles are always in present tense, and are not
   followed by punctuation.

 - Prefix commit titles with the module name, followed by a colon and a
   space (unless modifying a file in the base directory).  When
   modifying cmake modules, prefix with "cmake".  So for example, if you
   are modifying the obs-ffmpeg plugin::

     obs-ffmpeg: Fix bug with audio output

   Or for libobs::

     libobs: Fix source not displaying

 - If you still need examples, please view the commit history.

Headers
-------

  There's no formal documentation as of yet, so it's recommended to read
  the headers (which are heavily commented) to learn the API.

  Here are the most important headers to check out::

    libobs/obs.h                Main header

    libobs/obs-module.h         Main header for plugin modules

    libobs/obs-source.h         Creating video/audio sources

    libobs/obs-output.h         Creating outputs

    libobs/obs-encoder.h        Implementing encoders

    libobs/obs-service.h        Implementing custom streaming services

    libobs/graphics/graphics.h  Graphics API

    UI/obs-frontend-api/obs-frontend-api.h
                                Front-end API

  If you would like to learn from example, examine the default plugins
  (in the <plugins> subdirectory).  All features of OBS Studio are
  implemented as plugins.
