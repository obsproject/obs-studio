# setup-sparkle

The setup-sparkle action downloads the specified version of Sparkle and sets it up for use on a GitHub Actions runner.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `version` | The desired Sparkle version to install on the GitHub Actions runner. | (Required) |
| `checksum` | The expected SHA-256 checksum of the downloaded Sparkle release. | (Required) |
| `url` | The GitHub URL to a Sparkle repository with the released version. | (Default) [^1] |

[^1]: `https://github.com/sparkle-project/Sparkle/releases/download`

### Outputs

| Output | Description |
|:------:|-------------|
| `path` | The path to the directory in which Sparkle has been installed. |

## Common Usage

The version and checksum information are commonly tracked in the project's `CMakePresets.json` file, which can be parsed natively in Powershell or via `jq` to yield the expected information.

```yaml
    - name: Setup Sparkle
      id: setup-sparkle
      uses: ./.github/actions/publish-obs/setup-sparkle
      with:
        version: 2.7.1
        checksum: ad55747587873f7ab1af7b4b4602ad62845923b6f160acdca30c2e788f372d90
        nsis-checksum: 0ea38439f11005102b40d3df1cbeb9f0987b2dbb798d037378fe7662c157fb75
        url: https://github.com/sparkle-project/Sparkle/releases/download
```

## Notes

> [!IMPORTANT]
> This action requires a macOS GitHub Actions runner.

Sparkle does not require to be installed and is simply extracted into a managed location on the GitHub Actions runner, which is then provided as an output by the action.
