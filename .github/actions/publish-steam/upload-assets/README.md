# upload-assets

The upload-assets action builds a Steam release with prepared assets and uploads it to Steam.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `dry-run` | A boolean value to indicate whether to "dry-run" a Steam upload. | `false` |
| `steam-secret` | A client secret for authentication with Steam. | (Required) |
| `steam-user` | A user name for authentication with Steam. | (Required) |
| `steam-password` | A password for authentication with Steam. | (Required) |
| `branch-name` | The Steam release branch to publish the build to. | `stable` |
| `use-playtest` | A boolean value to indicate whether the build should be pushed as a playtest build. | `false` |
| `path` | The path to the directory with prepared Steam release assets. | `runner.temp` |

### Outputs

The action has no outputs.

## Common Usage

This action should be used in combination with `prepare-asset` and `setup-branches` as those will set up OBS Studio builds as required by the Steam build system and also detect the appropriate branch based on the GitHub Actions workflow event.

```yaml
      - name: Upload Steam Assets
        uses: ./.github/actions/publish-steam/upload-assets
        with:
          path: ${{ format('{0}/steam_asset', runner.temp) }}
          dry-run: true
          steam-secret: ${{ secrets.steam-client-secret }}
          steam-user: ${{ secrets.steam-client-user }}
          steam-password: ${{ secrets.steam-client-password }}
          branch-name: stable
          description: '35.0.0'
```

## Notes

Secrets should always be stored as environment secrets (and not repository secrets) as this allows a project to require approval by organization members before an associated workflow actually executes and accesses these secrets.

## Developer Notes

The output path of `steamcmd` is hard-coded in the `.vdf` files used when building the Steam packages and uses `build` from the working-directory in which the tool is invoked.

The action is cross-platform and thus uses a Bash script under the hood. As the script uses modern Bash features (mainly associate arrays), a more recent version of Bash is automatically installed on macOS GitHub Actions runners (which only come with Bash v3 by default).

* File paths are handled in their UNIX variant and converted from and to Windows format at the input/output edge of the Bash script.
