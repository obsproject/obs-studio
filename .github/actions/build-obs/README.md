# build-obs Action

The build-obs action runs a standardized build of OBS Studio for the platform of the GitHub Actions runner it is called on. It uses dedicated CI presets present in the project's `CMakePresets.json` file to inherit the same build settings as local builds, with compile warnings elevated to errors by default.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `config` | The CMake-style build configuration to use for building OBS Studio. See Notes. | `RelWithDebInfo` |
| `architecture` | The CPU architecture to build OBS-Studio for. See Notes. | `REQUIRED` |
| `codesign-ident` | The Apple Developer ID to use for code signing on macOS. | `-` |
| `codesign-team`  | The Apple Developer team ID to use for code signing on macOS. | `''` |
| `provisioning-profile-id` | The UUID of the provisioning profile to use for macOS builds. See Notes. | `''` |
| `analyze` | A boolean value to indicate whether to use a Clang `analyze` build. | `false` |
| `path` | The parent path for the destination directory of the generated build system. | `runner.temp` |
| `xcode-version` | An Xcode version number to select a specific Xcode version preinstalled on the runner. | `''` |
| `xcode-cas-path` | The path to an Xcode compilation cache. | `''` |
| `working-directory` | The path to a directory with an OBS Studio checkout for the action to operate on. | `github.workspace` |

### Outputs

| Output | Description |
|:------:|-------------|
| `analyzer-output-path` | The path to the directory with generated SARIF output files. |

## Common Usage

While the action will use the GitHub Actions runner operating system for platform selection, it will not automatically select an architecture, which needs to be provided as a required input.

```yaml
      - name: Build OBS Studio
        id: build
        uses: ./.github/actions/build-obs
        env:
          SOME_BUILD_VARIABLE: 'ON'
        with:
          config: RelWithDebInfo
          target: arm64
          codesign-ident: 'My Developer Name (<Hash>)'
          codesign-team: '<Team ID>'
          provisioning-profile-id: '<Some UUID>'
          xcode-version: 26.6
          xcode-cas-path: ${{ format('{0}/Compilation-Cache.noindex', runner.temp) }}
```

## Notes

Some inputs have validity constraints, not all of which are enforced immediately (instead the action permits the underlying build system generation or compilation to fail):

* `config` needs to be a valid CMake build configuration, so either `Debug`, `RelWithDebInfo`, `Release`, or `MinSizeRel`.
* `architecture` needs to be either `x86_64` or `arm64`.
  * `x86_64` will be automatically changed to `x64` for Windows builds.
  * `arm64` is only fully supported on macOS and Windows. Using it for builds on Linux GitHub Actions runners will lead to undefined behavior.
* If `xcode-version` is provided, the version needs to be installed on the GitHub Actions runner. The action will fail early if this condition is not met.
* Compilation caches are neither restored nor saved automatically by the action.

## Developer Notes

The action effectively serves as an automatic launcher of `CMake` to create a build system appropriate for each supported platform (Xcode on macOS, Visual Studio on Windows, Ninja on Ubuntu) and also runs compilation of the project.

* On macOS the specified Xcode version and location of the compilation cache are set up automatically before creation of the build system.
* On Ubuntu all dependencies required for build system generation as well as all dependencies for building OBS Studio are automatically installed using `apt-get`.
* `Ccache` is used to speed up Ubuntu builds on GitHub Actions if a preexisting cache had been restored.
  * The expected location of the compilation cache for OBS Studio builds is a directory named `.ccache` in the GitHub Actions runner's `temp` directory.
* File paths are handled in their UNIX variant and converted from and to Windows format at the input/output edge of the Bash script.
