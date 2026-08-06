# prepare-release-assets

The prepare-release-assets action downloads and prepares available workflow artifacts for an OBS Studio release.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `artifact-name` | A prefix or pattern used to select assets from available workflow artifacts. | `obs-studio-*' |
| `version` | The semantic version for which the assets should be prepared. | `true` |
| `download-artifacts` | A boolean value to indicate whether the action should download the artifacts automatically. | `false` |
| `working-directory` | A path to an optional working directory for the action. | `github.workspace` |

### Outputs

| Output | Description |
|:------:|-------------|
| `release-files` | A JSON string representing an array of all prepared assets. |
| `checksum-file` | The path to the generated markdown file with checksums for all prepared assets. |

## Common Usage

This action is commonly used in combination with `check-version-tag` and `generate-short-sha` to yield the version string and commit-ish used by generated artifacts.

```yaml
      - name: Generate Semver From Tag6
        id: semver
        uses: ./.github/actions/check-version-tag
        with:
          ref: ${{ github.ref_name }}
          fail-on-mismatch: true

      - name: Generate Short GitHub SHA
        id: short-sha
        uses: ./.github/actions/generate-short-sha

      - name: Prepare Release Assets
        id: prepare
        uses: ./.github/actions/publish-obs/prepare-release-assets
        with:
          artifact-name: ${{ format('obs-studio-*-{0}*', steps.short-sha.outputs.sha) }}
          version: ${{ steps.semver.outputs.version }}
          download-artifacts: true
```

## Notes

The action uses a predefined pattern to identify workflow assets:

```
obs-studio-<platform>-<architecture>-*<optional: asset type>.<extension>
```

This pattern would match any of the following artifact names:

* `obs-studio-windows-x64-<commit-ish>.zip`
* `obs-studio-macos-arm64-<commit-ish>-debug-symbols.tar.xz`
* `obs-studio-ubuntu-26.04-x86_64-<commit-ish>-sources.tar.gz`

The action itself only selects a specific subset of available workflow artifacts matching this pattern:

* macOS and Windows artifacts with the `-signed` suffix are used as application packages.
* Ubuntu artifacts without any suffix are used as application packages.
* All artifacts with the `-debug-symbols` suffix are used as debug symbols.
* macOS and Windows artifacts with the `-plugin-dev` suffix are used as plugin development libraries.
* Ubuntu artifacts with the `-sources` suffix are used as tarballs.
* Windows artifacts with the `-installer` suffix are used as installation packages.

Detected artifacts are renamed using another pattern:

```
OBS-Studio-<version>-<platform>-<architecture>-<type>.<extension>
```

This will result in the following prepared asset names:

* `OBS-Studio-xx.yy.zz-Windows-x64-Installer.exe` - Windows installation program
* `OBS-Studio-xx.yy.zz-macOS-Apple.dmg` - macOS Apple Silicon-based Mac disk image
* `OBS-Studio-xx.yy.zz-Ubuntu-26.04-x86_64-Sources.tar.gz` - Ubuntu tarball
* `OBS-Studio-xx.yy.zz-Windows-ARM64-PDBs.zip` - Windows debug symbols
* `OBS-Studio-xx.yy.zz-macOS-Intel-dSYMs.tar.xz` - macOS Intel-based Mac debug symbols

The exact list of generated files is provided as an output of the action.

## Developer Notes

For simplicity's sake, the action uses https://github.com/actions/download-artifact with the provided pattern to download all artifacts before picking only the assets required for a release. This means that the most efficient way to use this action is to only invoke it for workflow actions that produce OBS Studio builds and little other artifacts.

* If both unsigned and signed assets are available, signed assets will "overwrite" unsigned assets for the release.
