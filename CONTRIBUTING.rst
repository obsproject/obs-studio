Contributing
============

Quick Links for Contributing
----------------------------

- Compiling and building OBS Studio:
  https://github.com/obsproject/obs-studio/wiki/Install-Instructions

- Our bug tracker:
  https://github.com/obsproject/obs-studio/issues

- Discord Server: https://obsproject.com/discord

- Development chat: #development on the Discord server (see above)
 
- Development forum:
  https://obsproject.com/forum/list/general-development.21/

- Developer/API Documentation:
  https://obsproject.com/docs

- To contribute language translations, do not make pull requests.
  Instead, use crowdin.  Read here for more information:
  https://github.com/obsproject/obs-studio/wiki/How-To-Contribute-Translations-For-OBS

- To add a new service to OBS Studio please see the service submission guidelines:
  https://github.com/obsproject/obs-studio/wiki/Service-Submission-Guidelines

General Guidelines
------------------

- The OBS Project uses English as a common language. Please ensure that any
  submissions have at least machine-translated English descriptions and titles.

- Templates for Pull Requests and Issues must be properly filled out. Failure
  to do so may result in your PR or Issue being closed. The templates request
  the bare minimum amount of information required for us to process them.

- Contributors to the OBS Project are expected to abide by the OBS Project Code 
  of Conduct: https://github.com/obsproject/obs-studio/blob/master/COC.rst



Coding Guidelines
-----------------

- OBS Studio uses kernel normal form (linux variant), for more
  information, please read here:
  https://github.com/torvalds/linux/blob/master/Documentation/process/coding-style.rst

- Avoid trailing spaces.  To view trailing spaces before making a
  commit, use "git diff" on your changes.  If colors are enabled for
  git in the command prompt, it will show you any whitespace issues
  marked with red.

- Tabs for indentation, spaces for alignment.  Tabs are treated as 8
  columns wide.

- 120 columns max

- Comments and names of variables/functions/etc. must be in English

- Formatting scripts (macOS/Linux only) are available `here <./build-aux>`__

As of December 2024, we have done `a major reorganization and cleanup of the 
frontend <https://github.com/obsproject/obs-studio/pull/11622>`_. We are 
working towards implementing the following additional guidelines for C++ 
code:

  **Note**: Existing code was *not* updated to follow these guidelines.
  We ask that contributors bring old code in line with these rules 
  incrementally when making changes.

- Class names use **CapitalCase**, class methods use **camelCase**

- Class source files carry the name of their classes

- Class headers contain only their own class' definition

- Class headers only include other classes' interfaces (via their own
  header files) that the compiler needs, otherwise forward declarations are used

- Free-standing functions are put in an anonymous namespace (As opposed to
  `static` functions)

  - This also solves the issue of free-standing functions being sprinkled 
    throughout the source code and puts them in a single self-contained space.
    This also makes it obvious which utility functions might already exist and
    encourage re-use instead of adding yet another variant in another translation
    unit.

- The `inline` decoration should be used according to its actual meaning in 
  C++

  - That a function definition is provided "in line" (It has no direct 
    bearing on the code inlining behaviour of compilers).
  - It should never be used in source files and should only ever be used in 
    header files, and even there it should be used with all its associated 
    caveats applied

- Free-standing C++ functions should use **camelCase**, not **snake_case** 
  (Which instead should be used to clearly identify C functions vs C++ functions)

Naming Conventions
^^^^^^^^^^^^^^^^^^

- Clearly spell out words and avoid unnecessary abbreviations (e.g., no 
  AdvAudio, but AdvancedAudio).

- Ensure a consistent use of capitalisation across the codebase (Avoid having 
  both Youtube and YouTube)

- Make the purpose of functions explicit by their name itself, e.g., 
  nudgePreviewItem() or isOutputActive(). Avoid ambiguous method names like 
  Active() or Nudge()

- Use Get, Set, and Is prefixes to clearly communicate whether a method 
  gets, sets, or checks a state/status.

- Don't rely on magic global variables or extern symbols that also "magically" 
  have to exist in the global namespace. Encapsulate data with functionality and 
  use APIs to access data or delegate functionality to encapsulated classes 
  entirely.



Commit Guidelines
-----------------

- OBS Studio uses the 50/72 standard for commits.  50 characters max
  for the title (excluding module prefix), an empty line, and then a
  full description of the commit, wrapped to 72 columns max.  See this
  link for more information: http://chris.beams.io/posts/git-commit/

- Make sure commit titles are always in present tense, and are not
  followed by punctuation.

- Prefix each commit's titles with the module name, followed by a colon
  and a space (unless modifying a file in the base directory).  After
  that, the first word should be capitalized.

  So for example, if you are modifying the obs-ffmpeg plugin::

    obs-ffmpeg: Fix bug with audio output

  Or for libobs::

    libobs: Fix source not displaying

  Note: When modifying cmake modules, just prefix with "cmake".

- If you still need examples, please view the commit history.

- Commit titles and descriptions must be in English

AI/Machine Learning Policy
--------------------------

AI/machine learning systems such as those based on the GPT family (Copilot, 
ChatGPT, etc.) are prone to generating plausible-sounding, but wrong code that
makes incorrect assumptions about OBS internals or APIs it interfaces with.

This means code generated by such systems will require human review and is 
likely to require human intervention. If the submitter is unable to undertake
that work themselves due to a lack of understanding of the OBS codebase and/or
programming, the submission has a high likelihood of being invalid.
Such invalid submissions end up taking maintainers' time to review and respond
away from legitimate submissions.

Additionally, such systems have been demonstrated to reproduce code contained
in the training data, which may have been originally published under a license
that would prohibit its inclusion in OBS.

Because of the above concerns, we have opted to take the following policy
towards submissions with regard to the use of these AI tools:

- Submissions created largely or entirely by AI systems are not allowed.

- The use of GitHub Copilot and other assistive AI technologies is heavily
  discouraged.

- Low-effort or incorrect submissions that are determined to have been
  generated by, or created with aid of such systems may lead to a ban from
  contributing to the repository or project as a whole.
