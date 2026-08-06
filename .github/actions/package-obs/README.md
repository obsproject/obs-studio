# package-obs

The package-obs action produces platform-specific archives or images of an OBS Studio build produced on a GitHub Actions runner.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `config` | The CMake-style build configuration to use for building OBS Studio. See Notes. | `Release` |
| `architecture` | The CPU architecture to build OBS-Studio for. See Notes. | (Required) |
| `package` | A boolean value to indicate whether platform-specific packages should be created instead of a compressed archive. | `false` |
| `output-name` | A custom output name to use for the generated packages or compressed archives. | `''` |
| `path` | The path that contains the build directory to generate packages or compressed archives from. | `runner.temp` |
| `working-directory` | A path to an OBS Studio checkout. | `github.workspace` |

### Outputs

This action has no outputs.

## Common Usage

This action is designed to be used as a companion to the `build-obs` action, which produces the builds in the same default directory that this action expects as its "input".

```yaml
      - name: Package OBS Studio
        id: package
        uses: ./.github/actions/package-obs
        with:
          config: Release
          target: arm64
          package: false
          output-name: obs-studio-arm64-release
```

## Notes

These inputs are required for the action to find the appropriate build directories produced by the `build-obs` action and thus should use the same values.

* `config` needs to be a valid CMake build configuration, so either `Debug`, `RelWithDebInfo`, `Release`, or `MinSizeRel`.
* `architecture` needs to be either `x86_64` or `arm64`.
  * `x86_64` will be automatically changed to `x64` for Windows builds.
  * `arm64` is only fully supported on macOS and Windows. Using it for builds on Ubuntu GitHub Actions runners will lead to undefined behavior.
* When `package` is set to `true`, platform-specific packages are produced:
  * macOS GitHub Actions runners will produce macOS disk images with the `.dmg` suffix.
  * Ubuntu GitHub Actions runners will produce Ubuntu-style Debian packages with the `.deb` suffix.
  * Windows GitHub Actions runners will not produce any Windows-specific packages.

When the `Release` configuration is used, the action assumes that additional packages for an OBS Studio release need to be produced:

* macOS-based and Windows GitHub Actions runners will produce a "plugin development" package which contain `libobs` and `obs-frontend-api` for use in plugin development.

> [!NOTE]
> Ubuntu GitHub Actions runners can also produce plugin development libraries, but plugin developers should commonly use the libraries provided by their system package managers via a package like `libobs-dev` or `obs-studio-dev`.

* macOS-based and Ubuntu GitHub Actions runners will produce separate debug information in either `.dSYM` or `.ddeb` format.
* Ubuntu GitHub Actions runners will produce a "tarball" of all the sources used to produce the packaged build.

## Developer Notes

Windows and Ubuntu packages are produced via `CPack`, while macOS packages use a custom procedure implemented in this action to generate the disk image with all the Finder customization canonically expected by users on macOS.
