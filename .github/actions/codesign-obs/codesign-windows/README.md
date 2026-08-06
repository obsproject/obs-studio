# codesign-windows Action

The codesign-windows action code signs an existing OBS Studio Windows build provided as a `.zip` archive.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `path` | A path to an OBS Studio build archive in `.zip` format present on the GitHub Actions runner. | (Required) |

### Outputs

| Output | Description |
|:------:|-------------|
| `path` | The path to a new archive in `.zip` format with the code signed OBS Studio build. |

## Common Usage

The action requires a Windows GitHub Actions runner and the Google Cloud authentication setup to have succeeded before calling this action.

```yaml
      - name: Set Up Code Signing
        ...

      - name: Sign Windows Build
        id: codesign
        uses: ./.github/actions/codesign-obs/codesign-windows
        with:
          path: ${{ format('{0}/obs-studio-Windows-x64.zip', runner.temp) }}
```

Do not rely on the action to code sign the build archive in place, use the `path` output instead to unambiguously identify the output archive on the GitHub Actions runner.

## Notes

> [!IMPORTANT]
> The action requires a Windows GitHub Actions runner.

The `codesign-obs/setup-windows` action can be used to set up the Google Cloud credentials on the GitHub Actions runner before running this action.

The game capture hook is separately code signed using an RSA-based certificate due to Microsoft's post-quantum cryptography requirements.

## Developer Notes

This action uses `signtool` to apply code signing to all code files (executable and shared libraries) present in an OBS Studio build.

For signing the entire build, a chunk size of 5 files at a time is used to mimic the behavior of `bouf`. While the reason for this exact chunk size is lost to time, the action uses this value as a starting point.

* File paths are handled in their UNIX variant and converted from and to Windows format at the input/output edge of the Bash script.
