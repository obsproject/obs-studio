# lint-flatpak-manifest

The lint-flatpak-manifest action runs `flatpak-builder-lint` on a `flatpak-builder` manifest or a build directory.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `artifact` | The type of artifact to run `flatpak-builder-lint` on. Available values are `builddir`, `repo`, `manifest`, or `appstream`. | (Required) |
| `path` | A path to a `flatpak-builder` manifest or a Flatpak build directory. | (Required) |
| `working-directory` | A path to the root directory to base the manifest or build directory path on. | `github.workspace` |

### Outputs

This action has no outputs.

## Common Usage

```yaml
    - name: Validate Flatpak manifest
      uses: ./.github/actions/lint-obs/lint-flatpak-manifest
      with:
        artifact: manifest
        path: build-aux/com.obsproject.Studio.json

    ...

    - name: Validate build directory
      uses: ./.github/actions/lint-obs/lint-flatpak-manifest
      with:
        artifact: builddir
        path: flatpak_app
```

## Notes

> [!IMPORTANT]
> The action requires a Linux GitHub Actions runner.

The action needs to run in an environment where `flatpak-builder-lint` is available, which is commonly available via Docker images provided by the Flathub organization.

## Developer Notes

The action mainly serves as a convenience wrapper around the `flatpak-builder-lint`, which needs to be present on the GitHub Actions runner (or the container image) already. Since the command-line tool gained the `--gha-format` argument, no additional parsing of the tool's output is necessary anymore.
