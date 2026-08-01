# validate-flatpak-manifest

The validate-flatpak-manifest action parses the Flatpak manifest available in an OBS Studio checkout and checks if dumping it again introduces formatting errors.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `path` | The path to the Flatpak manifest file to lint. | `DEFAULT` [^1] |
| `fail-on` | A string indicating the fail condition to use by the action. Available values are `never`, `fast`, and `error`. | `never` |
| `working-directory` | A path to the root directory to base a relative file path on. | `github.workspace` |

[^1]: `build-aux/com.obsproject.Studio.json`

### Outputs

This action has no outputs.

## Common Usage

The action is designed to handle the output of the `check-changes` action, so that they can be used in combination with each other.

```yaml
      - name: Check for Changed Files
        id: checks
        uses: ./.github/actions/check-changes
        with:
          pathspec: 'build-aux/com.obsproject.Studio.json'
          filter: 'ACM'

      - name: Validate Flatpak Manifest
        if: fromJSON(steps.checks.outputs.has-changed-files)
        uses: ./.github/actions/lint-obs/validate-flatpak-manifest
        with:
          fail-on: error
```

## Notes

> [!IMPORTANT]
> The action requires a Linux GitHub Actions runner.

The linter result is indicated by the result of the action itself. If the manifest passed linting, the action will succeed without error. If `fail-on` is set to anything but `never`, linting failure will make the action fail as well.

## Developer Notes

The action makes use of the linter script available in OBS Studio's `build-aux` directory and is thus not entirely standalone.

The linter script itself simply parses the manifest file into a Python dictionary and then dumps it back into a formatted JSON string using 4 spaces for indentation.

Linting will fail if this produces a different JSON string than the current manifest file.
