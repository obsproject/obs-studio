# setup-pvs-studio Action

The setup-pvs-studio action downloads the specified version of PVS Studio from the provided download location and installs it on the Windows-based runner.

PVS Studio requires a valid license key to work, whose details need to be provided the the action.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `versione` | The PVS Studio version to install.| `REQUIRED`|
| `checksum` | The SHA-256 checksum of the downloaded PVS Studio installer. | `REQUIRED`|
| `url`| The URL of the PVS Studio setup program required for installation. | (Default Download URL) [^1] |
| `user` | The PVS Studio license user name. | `REQUIRED`|
| `license` | The PVS Studio license key. | `REQUIRED`|

[^1]: https://files.pvs-studio.com/PVS-Studio_setup.exe

### Outputs

The action has no outputs.

## Common Usage

The action will not discover potential PVS Studio metadata present in the project. This information has to be parsed independently before invoking the action.

```yaml
      - name: Create Action Inputs
        id: pvs-studio-data
        run: |
          {
            echo "version=4.5"
            echo "checksum=123"
            echo "user=<license user name>"
            echo "license=<license key>"
          } >> "${GITHUB_OUTPUT}"

      - name: Set Up PVS Studio
        id: analyze
        uses: ./.github/actions/analyze-obs/setup-pvs-studio
        with:
          version: ${{ steps.pvs-studio-data.outputs.version }}
          checksum: ${{ steps.pvs-studio-data.outputs.checksum }}
          user: ${{ steps.pvs-studio-data.outputs.user }}
          license: ${{ steps.pvs-studio-data.outputs.license }}
```

## Notes

> [!IMPORTANT]
> The action requires a Windows GitHub Actions runner.

## Developer Notes

Even though the action accepts a `url` input to explicitly specify a download URL, this URL needs to match the current default download URL very closely, as this URL is the only known canonical download location of PVS Studio. The regular expression implemented in the Powershell script would thus need to be changed to allow different or updated download URLs.

* File paths are handled in their UNIX variant and converted from and to Windows format at the input/output edge of the Bash script.
