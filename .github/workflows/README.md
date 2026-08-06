# OBS Studio GitHub Actions Architecture

## General Notes

The current implementation of workflows and actions prefers "native composition" as a design goal. A set of principles was used to decide whether to use a workflow, a "large" action, or a "small" action:

* If a GitHub event is handled, use a workflow.
* If jobs need to run in parallel, but also on different runner operating systems, use a workflow.
* If the jobs are potentially necessary for more than one GitHub event, use workflow calls with a reusable workflow.
* If the jobs can run in parallel and can all run on the same runner operating system, use a "large" repository action with parallel step execution.
* If the action step is potentially necessary for more than one repository action, create a new "small" repository action for just this single job.
* If the step in an action requires a shell script with more than 10 lines, extract into a bespoke script file and call it from the action instead.
    * Always pass action inputs to scripts as environment variables and check their values if user provided (e.g. commit titles, labels, tag names, etc.)

This avoids repetition of code in different workflows and also potentially highlights conceptual issues or design issues, e.g. when two or more actions do almost the same thing, differentiated by maybe one or two different inputs.

Another important design goal was to **use native workflow syntax and expressions as much as possible**. Using workflow syntax rather than shell scripts has the benefit that they can be evaluated and checked by GitHub's infrastructure before the workflow is even triggered.

When shell scripts are necessary, the following rule set applies:

* Use Bash scripts by default
    * Try to use only features available in Bash 3 - this has the benefit of producing scripts that can run on all GitHub Actions runners out of the box.
    * When more recent Bash features are necessary, install Bash via Homebrew on macOS GitHub Actions runners.

> [!NOTE]
> Be aware that the version of Bash 5 installed by Homebrew on macOS GitHub Actions runners is potentially much more recent than the version available on Linux GitHub Actions runners or the `git-bash` version available on Windows GitHub Actions runners.

* Use [ShellCheck](https://www.shellcheck.net) to lint all Bash scripts and Bash snippets.
* For macOS-exclusive actions, Zsh scripts are allowed as it's the default shell on modern macOS versions and has saner handling of variables (no automatic glob expansion or word splitting).
* Powershell scripts should be avoided (see below)

> [!NOTE]
> Even though Powershell scripts can be used by GitHub Actions, their use is discouraged in OBS Studio's CI implementation. This is due to Powershell's flawed debug output:
>
> Both Bash and Zsh allow each script line to be printed before execution, which is enabled when the `RUNNER_DEBUG` environment variable is set by the scripts used in the implementation.
>
> Powershell's trace output has two severe flaws: It is overly verbose (and thus more difficult to parse in GitHub Actions log output) and is also truncated automatically, which leads to secret values being leaked to arbitrary levels. This is because GitHub Actions can only mask secret values logged in their entirety, but not partial output.
>
> As Bash and Zsh do not truncate their debug output, secret values are successfully masked, and thus their use is preferred over Powershell.

## Implemented GitHub Events

OBS Studio uses GitHub Actions to handle the following GitHub events:

* `pull_request` - emitted when pull requests are opened, edited, etc. in the repository.
* `push` - emitted for any direct push to a branch of the repository.
* `schedule` - emitted for workflows that contain a cron-like scheduling string and are automatically triggered by GitHub.
* `dispatch` - emitted for workflows manually triggered by a project member.
* `publish` - emitted for a tag-based GitHub release being published.

These events are handled by dedicated workflows, which themselves trigger other independent workflows that implement common functionality (e.g. building OBS Studio) to avoid code duplication between workflows.

GitHub's term for these workflows is "reusable workflows", which this document will also use to differentiate them from workflows that handle GitHub events.

## Workflow Dependency Chart

The following diagram visualizes the relationship between the different workflows:
```
  ┌──────────────┐        ┌──────┐         ┌──────────┐       ┌──────────┐       ┌─────────┐
  │ PULL-REQUEST │        │ PUSH │         │ DISPATCH │       │ SCHEDULE │       │ PUBLISH │
  └──────┬───────┘        └┬────┬┘         └────┬─────┘       └────┬─────┘       └────┬────┘
         │                 │    │               │                  │                  │
         │                 │    │               │                  │                  │
┌────────▼──────────┐      │    │        ┌──────▼────────┐  ┌──────▼────────┐  ┌──────▼───────┐
│ pull-request.yaml │┌─────┘    │        │ dispatch.yaml │  │ schedule.yaml │  │ publish.yaml │
└─────┬─┬───────────┘│          │        └──────────┬──┬─┘  └─┬────┬────┬───┘  └──────┬───────┘
      │ │ ┌──────────▼┐  ┌──────▼────────┐          │  │      │    │    │             │
      │ │ │ push.yaml │  │ push-tag.yaml │          │  │      │    │    │             │
      │ │ └─┬────┬────┘  └─────┬───┬─────┘          │  │  ┌───┴────▼────┴──────┐      │
      │ │   │    │             │   │                │  └──► publish-steam.yaml ◄──────┘
      │ │   │    │             │   │                │     └───┬─────────┬──────┘
      │ │ ┌─▼────┴────────────┐│   │        ┌───────▼─────────┴────┐    │
      │ └─► lint-project.yaml ││   │        │ analyze-project.yaml ◄────┘
      │   └──────┬────────────┘│   │        └─────────────────┬────┘
      │          └──────┐      │   │                          │
      │                 │      │   │                          │
      │                ┌▼──────▼───┴────────┐                 │
      └────────────────► build-project.yaml ◄─────────────────┘
                       └───────────┬────────┘
                                   │
                       ┌───────────▼────────────┐
                       │ code-sign-project.yaml │
                       └────────────────────────┘
```

This relationship is based on the project needs for each GitHub event:

* For pull requests, code formatting needs to be checked and whether the project (and documentation) can still be built without error.
* The same applies to (non-tag) pushes, which will commonly happen when a pull request is merged. The workflow thus checks whether the _merged_ code still leaves the project in a buildable state.
* Tag-based pushes usually happen when a new version of OBS Studio is released. The project is then built and generated output artifacts code signed on Windows and macOS (if the repository is configured to use code signing and the appropriate environment secrets are set up).
    * The tag push will automatically create a GitHub release _draft_, which needs to be manually published. Once that happens, the release assets are also published to Steam and Flathub, and update files are generated for Windows and macOS.
* The schedule workflow runs every night but skips most jobs if the head SHA of the main branch has not changed since the last time the workflow ran. If any changes are detected, GitHub Actions caches are deleted and the project is fully built to re-populate the caches. Static analysis jobs are also run to produce CodeQL reports, which are too costly to run during pull requests. Lastly the nightly builds are also built and pushed to Steam.
* Manual dispatches allow project members to "fix up" issues that might have ocurred when publishing a release: Update file generation and Steam publishing can be manually triggered when the corresponding git tag reference is used as the workflow event SHA.

## Repository Configuration Variables

The workflow behavior can be influenced by a set of configuration variables. Their default value is always `false`, disabling their functionality out of the box for any fork of the project.

| Variable Name | Description |
|---------------|-------------|
| `ENABLE_DOCUMENTATION_UPDATE` | Enables building of Sphinx-based documentation by workflows. |
| `ENABLE_FLATHUB_PUBLISH` | Enables publishing release builds to Flathub. |
| `ENABLE_L10N_UPDATE` | Enables uploads and downloads of localization files to and from Crowdin. |
| `ENABLE_PVS_STUDIO` | Enables PVS-Studio for static code analysis. |
| `ENABLE_SERVICE_CHECKS` | Enables automatic availability checks of streaming services. |
| `ENABLE_STEAM_PUBLISH` | Enables publishing of release and nightly builds to Steam. |
| `ENABLE_UPDATE_GENERATION` | Enables automatic generation of update files for Windows and macOS. |
| `ENABLE_WINDOWS_CODE_SIGNING` | Enables code signing for Windows builds. |

> [!NOTE]
> Most of these features require additional environment secrets to be set. These requirements are outlined in the documentation for each workflow.

## Deployment Environments

The workflows use a set of deployment environments to manage secrets either used by the build process or optional features enabled by configuration variables:

| Environment | Description |
|-------------|-------------|
| `pull` | Optionally contains OAuth client credentials, which enables their compilation. |
| `build` | Contains macOS code signing credentials as well as OAuth client credentials to create "feature-complete" builds of OBS Studio. |
| `code-signing` | Contains macOS code signing and notarization credentials, Windows code signing credentials, Sparkle private key, and Steam access credentials. |
| `cf-pages-deploy` | Contains Credentials to publish documentation to CloudFlare pages. |
| `nightly` | Optionally contains OAuth client credentials to enable static code analysis for their implementation, as well as PVS-Studio license information. |

Some environments also support additional configuration variables:

| Environment | Variable | Description
|--|--|--|
| `nightly` | `SERVICE_CHECK_PURGE_AGE` | Minimum amount of seconds a checked service has to be offline to be automatically removed. |
| `code-signing` | `ENABLE_MACOS_NOTARIZATION` | Boolean string to indicate whether macOS builds should also be notarized. |

## Required Actions Permissions

It is considered good practice to only allow specific 3rd-party actions to be used by GitHub Actions in a repository. The following list of 3rd party actions is used across all workflows and repository actions and needs to be added as a comma-separated list:

* `flatpak/flatpak-github-actions/flatpak-builder@401fe28a8384095fc1531b9d320b292f0ee45adb` - used by Flatpak build jobs.
* `flatpak/flatpak-github-actions/flat-manager@401fe28a8384095fc1531b9d320b292f0ee45adb` - used to publish Flatpak builds to Flathub.
* `obsproject/obs-crowdin-sync/download@4b488c7ced03aa109d9f12529bd91c26c54b3e89` - used for translation file download.
* `obsproject/obs-crowdin-sync/upload@4b488c7ced03aa109d9f12529bd91c26c54b3e89` - used for translation file upload.
* `yuzutech/annotations-action@00b2d488bcba3bd01014dc073d276ef4a45d5c6c` - used to annotate JSON schema validation errors.
* `google-github-actions/auth@7c6bc770dae815cd3e89ee6cdf493a5fab2cc093` - used to authenticate with Google Cloud Services for Windows code signing and package synchronization.
* `cloudflare/wrangler-action@ebbaa1584979971c8614a24965b4405ff95890e0` - used to update documentation on CloudFlare pages

## Detailed Workflow Descriptions

### pull-request.yaml

|  GitHub Event  |           Event Types           | Environments
|----------------|---------------------------------|---------------|
| `pull_request` | `opened, synchronize, reopened` | `pull` |

|          3rd-Party Actions          |                    Hash                    |
|-------------------------------------|:------------------------------------------:|
| https://github.com/actions/checkout | `9c091bb21b7c1c1d1991bb908d89e4e9dddfe3e0` |

This workflow runs for opened pull requests (depending on repository settings) targeting the `main` or `master` branch, except if the changes are limited to markdown files only. If the pull request is reopened or "synchronized" (usually triggered by a force-push to the source reference), the workflow is triggered as well.

When triggered, the workflow will itself use the [`lint-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/lint-project.yaml) and [`build-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/build-project.yaml) workflows to have the linters and compilers run in parallel. If any of them has any failures, the entire integration check will be considered a failure.

> [!IMPORTANT]
> The [`build-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/build-project.yaml) workflow requires an environment string as input for optional access to environment secrets.

Additionally, if documentation updates are enabled and any documentation files have been changed, the documentation pages are built to check that any changes to `sphinx` files do not contain breaking changes.

#### Order Of Events

1. `pull_request` event
2. `pull-request` workflow runs
    * `lint-project` is triggered
    * `build-project` is triggered
    * `update-documentation` job runs

### push.yaml

| GitHub Event | Branches |
|--------------|----------|
| `push` | `master, main, release/**` |

| 3rd-Party Actions | Hash |
|-------------------|:----:|
| https://github.com/actions/checkout | `9c091bb21b7c1c1d1991bb908d89e4e9dddfe3e0` |

This workflow runs for any direct push to a `main` or `master` branch and any push to a release branch (e.g. `release/32.0.0`). Just like the `pull-request` workflow, pushes that contain just changes to markdown files are ignored, the [`lint-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/lint-project.yaml) and [`build-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/build-project.yaml) workflows are used to lint the code changes and build the project with the latest commit of the push, and documentation is generated.

> [!IMPORTANT]
> The [`build-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/build-project.yaml) workflow requires an environment string as input for access to environment secrets, which is required for the following features:
>
> * Inclusion of OAuth service connection code.
> * macOS code signing

#### Order Of Events

1. `push` event.
2. `push` workflow runs.
3. First layer:
    * `lint-project` is triggered.
    * `build-project` is triggered.
    * `update-documentation` job runs.

### push-tag.yaml

| GitHub Event | Tags |
|--------------|----------|
| `push` | `*` |

| 3rd-Party Actions | Hash |
|-------------------|:----:|
| https://github.com/actions/checkout | `9c091bb21b7c1c1d1991bb908d89e4e9dddfe3e0` |
| https://github.com/actions/download-artifact | `3e5f45b2cfb9172054b4087a40e8e0b5a5461e7c` |
| https://github.com/cloudflare/wrangler-action | `ebbaa1584979971c8614a24965b4405ff95890e0` |

This workflow only runs for pushed tags and is commonly used for versioned releases of OBS Studio. The project is built to generate release assets first using the [`build-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/build-project.yaml) workflow. If enabled, the assets are then code signed (and optionally notarized) by calling the [`code-sign-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/code-sign-project.yaml) workflow and documentation is built and published to CloudFlare pages.

If all these jobs succeed, a new GitHub _draft_ release is created by the workflow with all available (and supported) release assets automatically uploaded to the release.

> [!IMPORTANT]
> The reusable workflows used by this workflow require an environment string as input for access to environment secrets:
> * [`build-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/build-project.yaml)
>   * Inclusion of OAuth service connection code.
>   * macOS code signing
> * [`code-sign-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/code-sign-project.yaml)
>   * Windows code signing
>   * macOS code signing and notarization

#### Order Of Events

1. `push` event.
2. `push` workflow runs.
3. First layer:
    * [`lint-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/lint-project.yaml) is triggered.
    * [`build-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/build-project.yaml) is triggered.
    * `update-documentation` job runs.
4. Second layer:
    * `deploy-documentation` runs if `update-documentation` is successful.
    * [`code-sign-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/code-sign-project.yaml) is triggered if `build-project` is successful.
5. Third layer:
    * `create-release` runs if `code-sign-project` is successful.

#### Pinned hash of `code-sign-project`

Depending on security settings for accessing Windows code signing credentials, the hash for the [`code-sign-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/code-sign-project.yaml) workflow needs to be pinned to a specific commit of the workflow file using a more specific workflow path in the call. This is not necessary when just using macOS code signing and notarization.

The detailed description for the workflow has more information on the repercussions and requirements introduced by this design.

### dispatch.yaml

| GitHub Event | Inputs |
|--------------|--------|
| `dispatch` | `job, steam-asset-macos-apple, steam-asset-macos-intel, steam-asset-windows-x64` |

| 3rd-Party Actions | Hash |
|-------------------|:----:|
| https://github.com/actions/checkout | `9c091bb21b7c1c1d1991bb908d89e4e9dddfe3e0` |
| https://github.com/actions/download-artifact | `3e5f45b2cfb9172054b4087a40e8e0b5a5461e7c` |
| https://github.com/actions/upload-artifact/merge | `043fb46d1a93c77aae656e7c1c64a875d1fc6a0a` |
| https://github.com/cloudflare/wrangler-action | `ebbaa1584979971c8614a24965b4405ff95890e0` |

This workflow needs to be dispatched manually by project members. Unlike other workflows in the project, this one requires a specific use case to be selected before dispatching, which effectively "selects" the kind of jobs that are actually run.

> [!NOTE]
> The workflow will run in the state of the repository of the selected git reference. This can lead to unexpected results: When a past git reference is chosen, all changes committed to the repository since the tag will not be picked up by the workflow.

The available options are:

* `codeql` - runs static code analysis using the Clang Static Analyzer and PVS-Studio (if enabled)
* `services-check` - checks availability of streaming services contained in the `services.json` file of the `rtmp-services` module.
* `download-lang-files` - downloads available translation updates from Crowdin.
* `update-docs` - builds documentation files and publishes them to CloudFlare pages (if enabled).
* `generate-appcasts` - generates Sparkle AppCast update files when provided a tag reference.
* `generate-updates` - generates OBS Studio updater files when provided a tag reference.
* `publish-steam`- builds a Steam variant of OBS Studio and publishes it when provided a tag reference. This job accepts additional inputs:
    * `steam-asset-macos-apple` - a URL pointing to an OBS Studio package for macOS and Apple Silicon SOCs.
    * `steam-asset-macos-intel` - a URL pointing to an OBS Studio package for macOS and Intel CPUs.
    * `steam-asset-windows-x64` - a URL pointing to an OBS Studio package for Windows and Intel CPUs.
    * These custom assets need to be named after the official release assets they are meant to replace.

> [!IMPORTANT]
> The reusable workflows used by this workflow require an environment string as input for access to environment secrets:
> * [`analyze-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/analyze-project.yaml)
>   * Inclusion of OAuth service connection code.
>   * macOS code signing
>   * (Optional) PVS-Studio license registration.
> * [`build-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/build-project.yaml)
>   * Inclusion of OAuth service connection code.
>   * macOS code signing

#### Order Of Events

1. `dispatch` event.
2. `dispatch` workflow runs.
3. First layer: Depending on the choice, one of these is triggered:
    * [`analyze-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/analyze-project.yaml) is triggered.
    * `services-availability` job runs.
    * `download-language-files` job runs.
    * `update-documentation` and `update-documentation-cloudflare` jobs run.
    * `generate-appcasts` job runs.
    * `generate-windows-updates` job runs.
    * [`publish-steam`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/publish-steam.yaml) is triggered.
4. Second layer:
    * `deploy-documentation` runs if `update-documentation-cloudflare` is successful.
    * `merge-appcasts` runs if `generate-appcasts` is successful.

### scheduled.yaml

| GitHub Event | Cron Spec |
|--------------|-----------|
| `schedule` | `17 0 * * *` |

| 3rd-Party Actions | Hash |
|-------------------|:----:|
| https://github.com/actions/checkout | `9c091bb21b7c1c1d1991bb908d89e4e9dddfe3e0` |

This workflow takes care of daily maintenance jobs, scheduled to run about 15 minutes past midnight UTC. To avoid unnecessary duplication, the workflow first checks the head SHA of its most recent run and only continues with most jobs if the SHA has changed.

The following jobs are executed:

* GitHub Actions compilation caches are removed.
* The entire project is built with full code signing and OAuth service configuration to re-populate the compilation caches.
* Clang Static Analyzer and (optionally) PVS-Studio are run to generate CodeQL reports.
* Availability of streaming services contained in the `services.json` file of the `rtmp-services` module is checked.
* Any updated language files are uploaded to Crowdin, if enabled.
* A "nightly" build of OBS Studio is produced and published to Steam (if enabled).

Availability of streaming services is checked on every nightly run even if the head SHA of the main branch has not changed.

> [!IMPORTANT]
> The reusable workflows used by this workflow require an environment string as input for access to environment secrets:
> * [`analyze-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/analyze-project.yaml)
>   * Inclusion of OAuth service connection code.
>   * macOS code signing
>   * (Optional) PVS-Studio license registration.
> * [`build-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/build-project.yaml)
>   * Inclusion of OAuth service connection code.
>   * macOS code signing
> * [`publish-steam`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/publish-steam.yaml)
>   * Steam access credentials

#### Order Of Events

1. `schedule` event.
2. `scheduled` workflow runs.
3. First layer:
    * `check-nightly-ref` job runs.
    * `services-availability` job runs (if feature is enabled).
4. Second layer:
    * `cache-cleanup` job runs.
    * `upload-language-files` job runs (if feature is enabled).
5. Third layer:
    * [`build-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/build-project.yaml) is triggered.
    * [`analyze-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/analyze-project.yaml) is triggered.
5. Fourth layer:
    * [`publish-steam`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/publish-steam.yaml) is triggered.

### publish.yaml

| GitHub Event | Event Types |
|--------------|-------------|
| `release` | `published` |

| 3rd-Party Actions | Hash |
|-------------------|:----:|
| https://github.com/actions/checkout | `de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| https://github.com/actions/upload-artifact | `043fb46d1a93c77aae656e7c1c64a875d1fc6a0a` |
| https://github.com/flatpak/flatpak-github-actions | `401fe28a8384095fc1531b9d320b292f0ee45adb` |

| Docker Containers | Hash |
|-------------------|:----:|
|[flathub-infra/flatpak-github-actions](https://github.com/flathub-infra/actions-images/pkgs/container/flatpak-github-actions) | `364e5ede018e821ba430849690649ac7ec43d082c29ba4be3d357c517262ea1f`|

This workflow runs when a GitHub release of OBS Studio is published based on a tag either on the main branch or release branch. This enables automatic generation of patch files or publishing of builds to app stores with a release version of the application.

The following jobs will be run:

* When enabled, a full build of the project for Flatpak is triggered and published to Flathub if it passes validations.
* Sparkle AppCast files are generated and merged for macOS builds.
* Updater files are generated for Windows builds.
* A new Steam release will be generated and published.

The generated files are then attached as workflow artifacts and can be used by project members to upload to the respective systems that require them.

> [!IMPORTANT]
> The reusable workflows used by this workflow require an environment string as input for access to environment secrets:
> * [`publish-steam`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/publish-steam.yaml)
>   * Steam access credentials

#### Order Of Events

1. `release` event.
2. `publish` workflow runs.
3. First layer:
    * `setup-flathub` job runs, if enabled.
    * `generate-appcasts` job runs, if enabled.
    * `generate-windows-updates` job runs, if enabled.
    * [`publish-steam`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/publish-steam.yaml) is triggered.
5. Third layer:
    * `merge-appcasts` job runs.

### build-project.yaml

|    GitHub Event   |
|-------------------|
|  `workflow_call`  |

| 3rd-Party Actions | Hash |
|-------------------|:----:|
| https://github.com/actions/checkout | `de0fac2e4500dabe0009e67214ff5f5447ce83dd` |
| https://github.com/actions/upload-artifact | `043fb46d1a93c77aae656e7c1c64a875d1fc6a0a` |
| https://github.com/actions/cache | `27d5ce7f107fe9357f9df03efb73ab90386fccae` |
| https://github.com/flatpak/flatpak-github-actions | `401fe28a8384095fc1531b9d320b292f0ee45adb` |

| Docker Containers | Hash |
|-------------------|:----:|
|[flathub-infra/flatpak-github-actions](https://github.com/flathub-infra/actions-images/pkgs/container/flatpak-github-actions) | `364e5ede018e821ba430849690649ac7ec43d082c29ba4be3d357c517262ea1f`|

This workflow is the "work horse" of OBS Studio's GitHub Actions setup. It uses a build matrix to generate 6 parallel jobs to compile the project for Windows (`x64` and `ARM64`), macOS (`arm64` and `x86_64`), Ubuntu 26.04 (`x86_64`), and Ubuntu 24.04 (`x86_64`). An additional independent job builds the project for Flatpak (`x86_64`).

The workflow is called by "parent" workflows that handle specific GitHub events and thus reacts to the current event it is called with:

* Default build settings use optimizations and embedded debug information (`RelWithDebInfo` in CMake parlance).
    * Note that both Windows (`.pdb`) as well as macOS (`.dSYM`) use separate debug information files for optimized builds by default.
* For `pull_request` events, the project is built with default build settings and no artifacts are generated by default. The `Seeking Testers` tag has to be added as a label to a pull request to enable artifact generation.
* For `push`  events, the project is built with default build settings, except for pushed tags, which switches to full optimizations (`Release`).
    * Package generation is enabled as well, creating `.zip` archives for Windows, `.dmg` disk images for macOS, and `.deb` packages for Ubuntu.
    * On macOS code signing will be enabled with an ad-hoc Apple Developer identity as a default fallback.
* For `workflow_dispatch` events, the project is built with default build settings.
* For `schedule` events, the project is built with default build settings.

To speed up builds, compilation caches are used on macOS GitHub Actions runners and Ubuntu GitHub Actions runners. The caches are based on the current head of the main branch and are refreshed every night. This potentially limits the amount of cache misses to changes introduced by a pull request or push and should still speed up compilation of unchanged or otherwise unaffected files.

> [!NOTE]
> Compilation caching uses `ccache` on Ubuntu and the built-in Xcode compilation cache on macOS (based on LLVM's CAS storage).

Every build produces at the very least a full build of OBS Studio, debug symbols, libraries required for plugin development, as well as a "tarball" of the actual sources used to build the project (Ubuntu only).

Not all generated artifacts are automatically uploaded (e.g., developer libraries are created on Linux, but only to check if the generation code succeeds), and generated packages are _not code signed_. The code signing workflow needs to be triggered (with assets generated by this workflow) to actually create code signed (and notarized) packages.

> [!NOTE]
> The workflow uses `x86_64` throughout to identify builds for Intel CPUs, including Windows builds. The canonical term used by Microsoft for 64-bit Intel-based builds is `x64`, but this term is not used "internally" by the workflows. Instead the translation happens automatically by involved actions when generating the actual build system or release assets.

#### Notes On Flatpak Builds

Flatpak builds manage their own caches, as all dependencies are built from scratch for a Flatpak artifact. As those dependencies change at a much slower pace, caching those between builds speeds up Flatpak bundle generation tremendously, but also requires more space on the runners and thus makes it necessary to remove elements from the runner's drive that are not necessary for a Flatpak build. This cleanup includes files related to:

* CodeQL
* Python
* GHCUp
* Android development environment
* .NET development environment
* Swift development environment

#### Order Of Events

1. `workflow_call` event.
2. `build-project` workflow runs.
3. First layer:
    * `build-obs` matrix job runs with the following combinations:
        * macOS arm64
        * macOS x86_64
        * Ubuntu 24.04 x86_64
        * Ubuntu 24.04 x86_64
        * Windows x86_64
        * Windows arm64
    * `flatpak-build` job runs.

### lint-project.yaml

| GitHub Event |
|--------------|
| `workflow_call` |

| 3rd-Party Actions | Hash |
|-------------------|:----:|
| https://github.com/actions/checkout | `de0fac2e4500dabe0009e67214ff5f5447ce83dd` |

This companion to the [`build-project`](https://github.com/obsproject/obs-studio/blob/master/.github/workflows/build-project.yaml) workflow runs linters depending on the files changed in the GitHub event that triggered the original workflow. Each linter runs individually and failure does not impact any other job in the workflow (this ensures that all changed files are fully linted). The implemented linters include:

|     Linter     |                Git Pathspec                |  Comment            |
|:--------------:|:------------------------------------------:|---------------------|
| `clang-format` | `'*.c' '*.h' '*.cpp' '*.hpp' '*.m' '*.mm'` | Checks formatting.  |
| `swift-format` | `'*.swift'` | Checks formatting.  |
| `gersemi` | `'*.cmake' '*CMakeLists.txt'` | Checks formatting.  |
| `zizmor` | `'.github/**/*.yaml' '.github/**/*.yml'`  | Checks correctness. |
| `xmllint` | `'frontend/forms/**/*.ui'` | Checks correctness. |
| Custom | `'build-aux/com.obsproject.Studio.json'` | Checks correctness. |
| Custom | `'plugins/win-capture/data/*.json'` | Checks correctness. |
| Custom | `'plugins/rtmp-services/data/*.json'` | Checks correctness. |

#### Order Of Events

1. `workflow_call` event.
2. `lint-project` workflow runs.
3. First layer:
    * `clang-format` job runs.
    * `swift-format` job runs.
    * `gersemi` job runs.
    * `zizmor` job runs.
    * `flatpak-validator` job runs.
    * `qt-xml-validator` job runs.
    * `compatibility-validator` job runs, if base reference or target reference is the `master` or `main` branch.
    * `services-validator` job runs, if base reference or target reference is the `master` or `main` branch.

### analyze-project.yaml

| GitHub Event |
|--------------|
| `workflow_call` |

| 3rd-Party Actions | Hash |
|-------------------|:----:|
| https://github.com/actions/checkout  | `9c091bb21b7c1c1d1991bb908d89e4e9dddfe3e0` |

This workflow runs static code analysis on Windows (using PVS-Studio) and macOS (using Clang Static Analyzer) and converts the generated analysis files into a single SARIF file as expected by GitHub for CodeQL reports.

The jobs in this workflow require additional token permissions to post CodeQL reports:

| Permission | Type | Comment |
|:----------:|:----:|---------|
| `contents` | `read` | Necessary to list commits and other repository data. |
| `security-events` | `write` | Necessary to post CodeQL security scanning reports. |

#### Order Of Events

1. `workflow_call` event.
2. `analyze-project` workflow runs.
3. First layer:
    * `windows` job runs.
    * `macos` job runs.

### code-sign-project.yaml

| GitHub Event |
|--------------|
| `workflow_call` |

| 3rd-Party Actions | Hash |
|-------------------|:----:|
| https://github.com/actions/checkout  | `9c091bb21b7c1c1d1991bb908d89e4e9dddfe3e0` |
| https://github.com/actions/upload-artifact  | `043fb46d1a93c77aae656e7c1c64a875d1fc6a0a` |
| https://github.com/actions/attest  | `59d89421af93a897026c735860bf21b6eb4f7b26` |

This workflow code signs macOS and Windows build packages and runs additional jobs with those packages:

* macOS packages are notarized, if the feature is enabled.
* Debug symbols are generated for Windows builds.
* A Windows installer is generated for Intel-based CPUs

> [!IMPORTANT]
> While macOS code signing and notarization credentials are available as environment secrets, Windows builds cannot be code signed using certificates installed on the runner (at least until the project can use Azure Artifact Signing).
>
> This is due to the following set of constraints:
>
> * The game capture hook is code signed with an EV certificate.
> * When using EV certificates for code signing, Microsoft requires the use of a Hardware Security Module (HSM) following the CA/Browser Forum requirements for the storage of OV/EV certificate private keys.
>
> Thus to be able to code sign binaries with an OV/EV certificate on Windows one has to choose a provider that both provides cloud-based HSM solutions and supports Microsoft's "Cryptography API: Next Generation" (CNG).
>
> Microsoft's `SignTool.exe` can then be set up to communicate using CNG with the provider, which will then use the private key stored in its HSM to generate a certificate signing request (CSR) which can then be used on the GitHub Actions runner to code sign the binaries.

The workflow can optionally be "pinned" by using a specific git SHA in the calling workflow when hash-based access controls are used. Note that GitHub then requires a more complete path in the form of `<owner>/<repository>/<path to workflow>@<SHA>` to be used.

> [!NOTE]
> Using SHA-based access controls requires modification of each _calling_ workflow to include the desired commit SHA but also requires using a fork-specific path to the workflow file. This can potentially lead to merge conflicts when pulling in upstream changes to those workflows.

The jobs in this workflow require additional token permissions:

| Permission | Type | Comment |
|:----------:|:----:|---------|
| `contents` | `read` | Necessary to list commits and other repository data. |
| `id-token` | `write` | Necessary to generate an attestation for the code signed build. |
| `attestations` | `write` | Necessary to generate an attestation for the code signed build. |

#### Order Of Events:

1. `workflow_call` event.
2. `code-sign-project` workflow runs.
3. First layer:
    * `code-sign-macos` job runs.
    * `code-sign-windows` job runs, if enabled.
4. Second layer:
    * `create-windows-installer` job runs, if enabled and `code-sign-windows` succeeded

### publish-steam.yaml

| GitHub Event |
|--------------|
| `workflow_call` |

| 3rd-Party Actions | Hash |
|-------------------|:----:|
| https://github.com/actions/checkout  | `9c091bb21b7c1c1d1991bb908d89e4e9dddfe3e0` |
| https://github.com/actions/download-artifact  | `3e5f45b2cfb9172054b4087a40e8e0b5a5461e7c` |
| https://github.com/actions/upload-artifact  | `043fb46d1a93c77aae656e7c1c64a875d1fc6a0a` |

This workflow extracts OBS Studio builds for Windows and macOS and copies the contained files into a directory structure required by Steam builds.

The assets are prepared in parallel by macOS and Windows jobs before being combined in a dedicated job to publish to Steam.

> [!IMPORTANT]
> This workflow should be called with an appopriate deployment environment identifier that provides access to Steam account credentials as well as a client secret for Steam guard token generation.

#### Order Of Events:

1. `workflow_call` event.
2. `publish-steam` workflow runs.
3. First layer:
    * `macos-build` matrix job runs for `x86_64` and `arm64`.
    * `windows-build` matrix job runs for `x86_64`.
4. Second layer:
    * `create-package` job runs.
