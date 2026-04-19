OBS Studio Contribution Guidelines
============

OBS Studio accepts contributions via public **pull requests**, which encompass changes made to the project as well as a written description of the changes made and the motivation behind them.

Pull requests can be opened by any user with sufficient contribution permissions on the platform chosen by the project. Likewise, any user with sufficient access permissions is able to read, comment, and suggest changes to a pull request, but the ultimate decision for or against adopting the suggested changes lies with **project members**.

OBS Studio has shown steady growth of popularity, complexity, and scope throughout its existence. Contributions that demonstrate they have taken the aspects outlined in this document into account (including the impact to the maintainability of the project) will potentially, but not necessarily, have a higher chance of adoption.

> [!NOTE]
> Past acceptance of a change, including “prior art” that exists in the codebase, does not guarantee that future, similar, changes will be accepted, as the “state of the art” (code style guidelines, best practices, language standard) is constantly evolving and the project consciously chooses to evolve with them.

These guidelines attempt to provide a set of general rules that any contributor can choose to follow and implement before opening a pull request to reduce review effort and increase its chances for adoption.

## General Contribution Guidelines

The canonical language used by the project is “American English”, which mainly applies to source code and non-translated material. Examples include:

* Commit messages
* Names of constants, variables, and types
* Source code comments
* File names

Appropriate templates are presented to the user when opening a new issue report or pull request on GitHub which are required to be fully filled out to be taken into consideration by the project.
The template might require checking off items that might not be applicable e.g., the requirement to have the code properly formatted does not apply when the contribution has not changed any source code. **It is permissible and preferred to tick the box regardless**.

Authors of contributions to the OBS Project are expected to abide by the “OBS Project Code of Conduct” available at https://github.com/obsproject/obs-studio/blob/master/COC.rst.

## Commit Authoring Guidelines

The project uses the “50/72” standard for commit messages, which requires that a commit title uses no more than 50 characters, while any consecutive line used for the commit description is allowed to use a maximum of 72 characters (with long lines manually broken up).

The “anatomy” of a correct commit message with description looks like this:

```
prefix: This is a commit title using 50 characters

This is a commit description line that is allowed to use up to 72 chars
```

Prefixes are commonly defined by the “module” the commit applies changes to, which has repercussions as to how commit contents are authored (see below).

* Changes made to code for “libobs” should use the “libobs” prefix, similarly changes made to “obs-ffmpeg” use “obs-ffmpeg”.
* Changes made to multiple modules in the “plugins” directory can use the “plugins” prefix, particularly if the changes are of a similar nature
* Changes made to files involved in continuous integration (CI) should use the “CI” prefix regardless of the directory name.
* Changes made to files used by the CMake build system should use the “cmake” prefix”

When in doubt about what the correct prefix for a change might be, the existing commit history might be of help, however project guidelines might have changed in the interim and thus a “past” commit message might not be acceptable for new contributions.

### Commit Messages

The commit message is the primary place for documentation of changes, not only as a summary of “what” was changed, but even more importantly “why” the changes were made and “why” specifically in the way they have been made.

As such the commit message should be written in such a way that it could easily pass as the “Description” and “Motivation and Context” required when opening a new pull request (and indeed platforms like GitHub will automatically copy the commit message into the opening post for a pull request with a single commit).

This ensures that documentation and context are encoded within the source tree and do not require external documents or platforms to ensure this information is not lost.

Many of these ideas match the ideas outlined in https://chris.beams.io/git-commit.

### Commit Content Authoring Guidelines

#### Code Style

All code contributions to the project need to follow code style guidelines appropriate to the languages used by the changed files. For C and C++ the project has chosen to adopt well-established code style guidelines as its baseline with specific exceptions and changes outlined in a [separate document](https://github.com/obsproject/obs-studio/blob/master/CODESTYLE.md).

The baseline rules for the common languages used by the project are:

* C code needs to follow the Linux Kernel Coding Style (https://github.com/torvalds/linux/blob/master/Documentation/process/coding-style.rst)
* C++ code needs to follow OBS Studios’ style guide. Familiarity with the Google C++ Style Guide (https://google.github.io/styleguide/cppguide.html) can serve as a baseline.
* Objective-C and Objective-C++ code needs to follow Apple’s ObjectiveC Conventions (https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/ProgrammingWithObjectiveC/Conventions/Conventions.html)
* Swift code needs to follow the API Design Guidelines (https://www.swift.org/documentation/api-design-guidelines/)
* **The column size limit for all code is 120 characters**

To help with the formatting of code, the project contains configuration files for code-formatters of several languages:

* `clang-format` can be used to correctly format C, C++, Objective-C, and Objective-C++ soure code
* `swift-format` can be used to correctly format Swift source code
* `gersemi` can be used to correctly format CMake source code

> [!NOTE]
> The “build-aux” directory in the project’s root directory contains scripts that can automate this process and are used by the project’s CI infrastructure to automatically check all contributions that changed corresponding source code files.

#### Commit Contents

The changes contained in a commit should attempt to represent logical “units of change” that make sense in isolation and do not necessarily require any preceding or following commits to work. Indeed if a single commit would potentially leave the project in a broken state (e.g., the project cannot be compiled, the application not run, or basic application functionality cannot function) then it is not a full “unit of change”.

* Changes to misspelled words or purely aesthetic changes should in principle not be individually capable of breaking the project in such a way and are permitted to be grouped into a single “large” commit
* If a larger change requires some foundation of fixes to the existing code to make sense, those fixes represent a “unit of change” that should be decoupled from the functional changes made on top of them. Even better, such a change should be encapsulated in its own, separate, pull request.

> [!IMPORTANT]
> During review and collaboration the need for additional changes can arise quickly. It is permitted (and preferred) to temporarily ignore these guidelines to quickly provide changes required to address review comments and add many small, iterative, commits to the pull request.
>
> Once all review comments have been addressed, the changes need to be squashed into existing commits or alternatively a new set of commits can be created from all accumulated changes to restore compliance with the guidelines again.

#### Co-Authorship

Sometimes an open pull request might have been "lost to time" even though its contents might still be a meaningful and positive change. When picking up such a change and authoring a new pull request to "finish it", the original author should be retained as much as possible:

* The original commit can be cherry-picked, which will retain the author information and only change the committer information
* If the original commit needs to be adjusted and changed as it will not apply to an updated code-base, its original author should be added as a "Co-Author" to the commit message.

```
libobs: Refactor audio implementation


Additional commit description
<..>
Co-authored-by: Name <git@address.TLD>
```

# AI/Machine Learning Policy

AI/machine learning systems such as Copilot, ChatGPT, and Claude, are prone to generating plausible-sounding, but wrong code that makes incorrect assumptions about OBS internals or APIs it interfaces with.

This means code generated by such systems will require human review and is likely to require human intervention. If the submitter is unable to undertake that work themselves due to a lack of understanding of the OBS codebase and/or programming, the submission has a high likelihood of being invalid. Such invalid submissions end up taking maintainers' time to review and respond away from legitimate submissions.

Additionally, such systems have been demonstrated to reproduce code contained in the training data, which may have been originally published under a license that would prohibit its inclusion in OBS.

Because of the above concerns, we have opted to take the following policy towards submissions with regard to the use of these AI tools:

* Submissions created largely or entirely by AI systems are not allowed.
* The use of GitHub Copilot and other assistive AI technologies is heavily discouraged.
* Low-effort or incorrect submissions that are determined to have been generated by, or created with aid of such systems may lead to a ban from contributing to the repository or project as a whole.
