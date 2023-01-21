Contributing
============

Quick Links for Contributing
----------------------------

- `Compiling and building OBS Studio <https://github.com/obsproject/obs-studio/wiki/Install-Instructions>`_.
  

- `Bug tracker <https://github.com/obsproject/obs-studio/issues>`_.
  

- `Discord Server <https://obsproject.com/discord>`_, (Development chat: #development).
 
- `Development forum <https://obsproject.com/forum/list/general-development.21/>`_.
 
- `Developer/API Documentation <https://obsproject.com/docs>`_.

- To contribute language translations, **do not make pull requests**.
  Instead, use **Crowdin**.  `Read here <https://obsproject.com/forum/threads/how-to-contribute-translations-for-obs.16327/>`_ for more information.
 

- To add a new service to OBS Studio please see the `service submission guidelines <https://github.com/obsproject/obs-studio/wiki/Service-Submission-Guidelines>`_.
  

Coding Guidelines
-----------------

- OBS Studio uses kernel normal form (linux variant). For more
  information, please read `here <https://github.com/torvalds/linux/blob/master/Documentation/process/coding-style.rst>`_.
  

- Avoid trailing spaces.  To view trailing spaces before making a
  commit, use "git diff" on your changes.  If colors are enabled for
  git in the command prompt, it will show you any whitespace issues
  marked with red.

- Use 'Tabs' for indentation and 'Spaces' for alignment.  Tabs are treated as 8
  columns wide.

- 80 columns max

Commit Guidelines
-----------------

- OBS Studio uses the 50/72 standard for commits.  50 characters max
  for the title (excluding module prefix), an empty line, and then a
  full description of the commit, wrapped to 72 columns max.  See this
  `link <http://chris.beams.io/posts/git-commit/>`_ for more information.

- Make sure commit titles are **always in present tense**, and are **not
  followed by punctuation**.

- Prefix each commit's title with the module name, followed by a colon
  and a space (unless modifying a file in the base directory).  After
  that, the first word should be capitalized.

  So for example, if you are modifying the obs-ffmpeg plugin, write in this format::

    obs-ffmpeg: Fix bug with audio output

  Or for libobs::

    libobs: Fix source not displaying

  **Note: When modifying cmake modules, just prefix with "cmake"**.

- If you still need examples, please view the commit history.

Conduct Guidelines
------------------

- Contributors to the OBS Project are expected to abide by the `OBS Project Code of Conduct <https://github.com/obsproject/obs-studio/blob/master/COC.rst>`_.
