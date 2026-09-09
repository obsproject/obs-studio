Aerium
======

Aerium is an independent fork of `OBS Studio <https://obsproject.com>`_,
maintained by `AeriumChris <https://github.com/AeriumChris>`_. The aim is to
build on OBS Studio's recording and streaming capabilities with additional
functionality, a customised interface, and simpler approaches to selected
workflows.

The project is starting from OBS Studio's existing foundation, not building
a new recording and streaming engine from scratch. Credit for that
foundation belongs to the OBS Project and its contributors. Aerium is not
an official OBS Project release or an endorsed OBS product.

Early Development
-----------------

**Aerium is in its very early stages.** So far, work has focused on setting
up the fork and documenting its direction. Aerium-specific application
features and interface changes have not been implemented yet.

The goals below describe what the project intends to explore, not features
that are already available or a committed release roadmap. Scope, design,
and priorities may change as development progresses.

Aerium does not yet have its own validated release or release process. For
everyday recording or production streaming, use an official
`OBS Studio release <https://obsproject.com/download>`_.

Project Goals
-------------

- **Additional functionality:** explore useful features that build on the
  capabilities already available in OBS Studio.
- **A customised interface:** develop Aerium's own interface and visual
  identity around practical recording and streaming workflows.
- **Simpler workflows:** identify where setup, controls, or common tasks
  can be made easier without unnecessarily removing useful flexibility.
- **A maintainable fork:** keep changes focused so that improvements and
  fixes from upstream OBS Studio remain practical to incorporate.

AI-Assisted Development
-----------------------

AI assistance, including GitHub Copilot, is part of Aerium's development
workflow from the outset. It has helped with repository setup and project
documentation. As application work begins, it will also support exploring
the OBS codebase, planning changes, drafting code, and investigating bugs.

This is AI-assisted development, not a claim that Aerium currently includes
AI-powered recording or streaming features. AI-generated suggestions can
be incorrect, incomplete, or unsuitable for the project. They need review
and appropriate testing before being relied on; generated code is not
evidence that a feature works.

Responsibility for project direction, code quality, security, and licensing
remains with the people maintaining and contributing to Aerium. AI tools
support that work; they do not replace engineering judgement or validation.

Development and Feedback
------------------------

The source and Aerium-specific work live in the
`Aerium repository <https://github.com/AeriumChris/Aerium>`_. Please direct
feedback and proposed changes for this fork there, rather than to OBS
Studio's support channels.

For source builds, the upstream
`OBS build instructions <https://github.com/obsproject/obs-studio/wiki/Install-Instructions>`_
are the starting point. Aerium-specific instructions will be documented as
the fork develops. Setting up this repository does not mean an Aerium build
has been independently validated.

Before proposing code changes, read the inherited
`contribution guidelines <CONTRIBUTING.md>`_,
`code style guidelines <CODESTYLE.md>`_, and `Code of Conduct <COC.rst>`_.
Some inherited documentation still refers to OBS Studio and its processes.

Upstream and Licensing
----------------------

- Official OBS Studio: https://obsproject.com
- Upstream source: https://github.com/obsproject/obs-studio
- OBS documentation and guides: https://github.com/obsproject/obs-studio/wiki
- Developer/API documentation: https://obsproject.com/docs
- Support the OBS Project: https://obsproject.com/contribute

Aerium retains OBS Studio's GNU General Public License v2 (or any later
version). See `COPYING <COPYING>`_ for the license and `AUTHORS <AUTHORS>`_
for upstream contributor credits. Modifications and redistribution must
respect the applicable license obligations; AI assistance does not change
those obligations.
