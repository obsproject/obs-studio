# setup-bouf

The setup-bouf action downloads a specified version of BOUF and sets it up for use on a GitHub Actions runner.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `version` | The desired BOUF version to install on the GitHub Actions runner. | (Required) |
| `checksum` | The expected SHA-256 checksum of the downloaded BOUF release. | (Required) |
| `nsis-checksum` | The expected SHA-256 ckechsum of the download NSIS support files used by BOUF. | (Required) |
| `url` | The GitHub URL to a BOUF repository with the released version. | (Default) [^1] |

[^1]: `https://github.com/obsproject/bouf/releases/download`

### Outputs

| Output | Description |
|:------:|-------------|
| `path` | The path to the directory in which BOUF has been installed. |

## Common Usage

The version and checksum information are commonly tracked in the project's `CMakePresets.json` file, which can be parsed natively in Powershell or via `jq` to yield the expected information.

```yaml
    - name: Setup BOUF
      id: setup-bouf
      uses: ./.github/actions/publish-obs/setup-bouf
      with:
        version: 3.5.1
        checksum: ad55747587873f7ab1af7b4b4602ad62845923b6f160acdca30c2e788f372d90
        nsis-checksum: 0ea38439f11005102b40d3df1cbeb9f0987b2dbb798d037378fe7662c157fb75
        url: https://github.com/obsproject/bouf/releases/download
```

## Notes

> [!IMPORTANT]
> This action requires a Windows GitHub Actions runner.

As BOUF requires no installation, the release is extracted into a managed location on the GitHub Actions runner, which is then provided as an output by the action.

An installed instance of BOUF with NSIS support files commonly uses the following directory scheme:

* `<installation prefix>/bin/bouf.exe` - the main BOUF executable.
* `<installation prefix>/nsis/` - the NSIS support files

Any invocations of BOUF and scripts used to generate the installation program might need to be adjusted or set up correctly to use these directories.

## Developer Notes

BOUF itself does not necessarily require the NSIS support files for most of its functionality, but the action does not assume the use case for which BOUF should be set up. For that reason the NSIS support files are always downloaded and extracted "just in case".

* File paths are handled in their UNIX variant and converted from and to Windows format at the input/output edge of the Bash script.
