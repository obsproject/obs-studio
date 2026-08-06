# prepare-asset

The prepare-asset action prepares a OBS Studio release asset for publishing on Steam.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `platform` | The platform to prepare the Steam asset for. See Notes. | `REQUIRED` |
| `architecture` | The CPU architecture to prepare the Steam for. See Notes. | `REQUIRED` |
| `path` | The path to the release asset to prepare. | `github.workspace` |

### Outputs

| Output | Description |
|:------:|-------------|
| `path` | The path to a directory with the prepared Steam release asset. |

## Common Usage

The action does not download assets itself, the `download-asset` action can be used in combination instead:

```yaml
      - name: Download Asset
        id: download
        uses: ./.github/actions/download-asset
        with:
          platform: macOS
          architecture: arm64
          path: ${{ format('{0}/steam_asset', github.workspace) }}

      - name: Prepare Asset
        id: prepare
        uses: ./.github/actions/publish-steam/prepare-asset
        with:
          platform: macOS
          architecture: arm64
          path: ${{ format('{0}/steam_asset', github.workspace) }}
```

## Notes

> [!IMPORTANT]
> The action requires a macOS GitHub Actions runner when preparing macOS release assets for Steam.

The action attempts to find a _release_ asset based on a name pattern:

```
OBS-Studio-<version>-<platform>-<architecture>.<zip or dmg>
```

The supported platforms are `macOS` or `Windows` and supported architectures are `arm64` or `x86_64` (`x64` on Windows). The action automatically translates the platform and architecture values into the appropriate release file names for detection of an available asset.

The release assets are extracted and copied into the directory structure expected by the Steam release including the additional support files.

## Developer Notes

The Steam build script expects macOS and Windows files to be placed in a bespoke directory structure, with helper script files added to appropriate locations:

```
<root>
    <steam-macos>
        <x86_64>
            OBS.app
        <arm64>
            OBS.app
        launch.sh
    <steam-windows>
        <bin>
        <obs-plugins>
        <data>
        <scripts>
            install.bat
            installscript.vdf
            uninstall.bat
        disable_updater
```

The action is cross-platform and thus uses a Bash script under the hood. As the script uses modern Bash features (mainly associate arrays), a more recent version of Bash is automatically installed on macOS GitHub Actions runners (which only come with Bash v3 by default).

* File paths are handled in their UNIX variant and converted from and to Windows format at the input/output edge of the Bash script.

