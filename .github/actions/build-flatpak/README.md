# build-flatpak Action

The build-flatpak action bundles a set of common steps for building a Flatpak bundle using the manifest available in the OBS Studio repository.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `architecture` | The CPU architecture to build OBS-Studio for. Available value is `x86_64`. | `REQUIRED` |
| `bundle` | A boolean value to indicate whether to actually build a bundle. If set to `false`, an available cached bundle is used instead. | `false` |
| `github-token` | The GitHub token required to check GitHub Actions caches for available cached bundles. | `github.token`|
| `working-directory` | The path to a directory with an OBS Studio checkout for the action to operate on. | `github.workspace` |

### Outputs

The action has no outputs.

## Common Usage

The action will attempt to detect if a compatible GitHub Actions cache entry is available for re-use and will then either download an available cache or create a new one based off the result of the bundle operation.

```yaml
      - name: Create Flatpak Bundle
        uses: ./.github/actions/build-flatpak
        with:
          architecture: x86_64
          bundle: true
```

## Notes

> [!IMPORTANT]
> The action requires a Linux GitHub Actions runner.

* The tool-chain required to build Flatpak manifests is commonly preinstalled on containers available by the Flathub organization for use on GitHub Actions.

## Developer Notes

The action makes use of https://github.com/flatpak/flatpak-github-actions to do the actual bundling. This action is also ultimately responsible for restoring or creating the GitHub Actions cache entry.

For convenience reasons, the action has been designed to re-use the default cache key used by the action, which currently uses the `flatpak-builder-<architecture>-<20-character-SHA>` pattern. Notably, the `architecture` token is second-to-last when used in the automatically generated cache key, but is automatically _appended_ as the last token to any manually provided cache key (potentially duplicating this information if already provided as part of the key).
