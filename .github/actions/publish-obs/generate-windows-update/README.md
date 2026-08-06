# generate-windows-update

The generate-windows-update action generates proprietary update files to use with OBS Studio's automatic updater on Windows.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `architecture` | The CPU architecture to generate updater files for. See Notes. | `REQUIRED`|
| `channel` | The update channel to generate updater files for. See Notes. | `stable` |
| `working-directory` | A path to an OBS Studio checkout. | `github.workspace` |

### Outputs

This action has no outputs.

## Common Usage

The action requires Windows code signing credentials to be set up on the GitHub Actions runner and thus should be used in conjunction with the `codesign-obs/setup-windows` action.

```yaml
      - name: Set Up Code Signing
        uses: ./.github/actions/codesign-obs/setup-windows
        with:
          gcp-identity-provider: ${{ secrets.gcp-identity-string }}
          gcp-account-name: ${{ secrets.gcp-account-name }}

      - name: Generate Windows Updater Files
        uses: ./.github/actions/publish-obs/generate-windows-update
        with:
          architecture: arm64
          channel: stable
```

## Notes

> [!IMPORTANT]
> The action requires a Windows GitHub Actions runner.

* The action needs to be run with a git tag reference.
* The action will fetch all recent builds available in a remote storage location to generate update files.
* The action will replace the "latest" build in a remote bucket with the workflow asset available for the current event and also add it to existing builds.
* Release notes are generated based on the body and subject lines on the tag reference.
* The generated updater files are not automatically uploaded to remote storage, but will be uploaded as a workflow artifact instead.


## Developer Notes

The action uses https://github.com/obsproject/bouf to generate the installation program and archives. BOUF and its dependencies are automatically installed on the GitHub Actions runner by the action.

The setup requirements are extracted from an OBS Studio `CMakePresets.json` file using the following `jq` query:

```typescript
  .configurePresets[]
  // Select the preset with the "dependencies" name
  | select(.name == "dependencies")
  // Select the "bouf" key from the "tools" object
  | .vendor["obsproject.com/obs-studio"].tools.bouf
  // Convert each {"some_key": "some_value"} pair into an array of
  // {"key": "some_key", "value": "some_value"} elements
  | to_entries
  // Convert each array element into a "some_key=some_value" string.
  | map("\(.key)=\(.value|tostring)")
  // Print each array item as separate output line for use as GitHub Actions output.
  | .[]
```

This allows `jq` to output key-value pairs directly in the format required by GitHub Actions.

* BOUF requires all binary dependencies to be installed regardless of which functionality is used, so NSIS and Pandoc need to always be installed.
  * The Pandoc version provided by `winget` installs into the local application data directory, the exact location is environment-dependant. So no fixed location can be provided in BOUF's config file and instead the `PATH` needs to be used.
* File paths are handled in their UNIX variant and converted from and to Windows format at the input/output edge of the Bash script.
