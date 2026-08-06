# create-github-release

The create-github-release action creates a GitHub release based on the provided git tag and the content provided via inputs.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `name` | The display name of the release. | `''` |
| `tag` | The git tag to base the release on. | `''` |
| `release-notes` | A multi-line string with the release notes to use as the body of the release. | `''` |
| `release-note-file` | The path to a file containing the body text for the release. | `'' ` |
| `generate-release-notes` | A boolean value to indicate whether release notes should be automatically generated based on the `previous-tag`. | `false` |
| `draft` | A boolean value to indicate whether the release should be created as a draft. | `true` |
| `is-prerelease` | A boolean value to indicate whether the release is created as a "pre-release". | `false` |
| `previous-tag` | The tag name of the release that is considered the "previous" release. Required for automatic release note generation. | `''` |
| `files` | A JSON string representing an array of files to attach to the release or a glob pattern. | `''` |
| `github-token` | The GitHub token required for the `gh` command-line utility to create or edit GitHub releases. The provided token needs to have the `contents: write` permission. | `github.token` |
| `working-directory` | The path to a directory to use as a base path for relative file paths. | `github.workspace` |

### Outputs

| Output | Description |
|:------:|-------------|
| `release-id` | The ID of the created or updated GitHub release. |
| `release-url` | The URL to the created or updated GitHub release. |
| `release-upload-url` | The upload URL to add additional release assets to the GitHub release. |
| `release-assets` | A JSON string representing an array of URLs for assets uploaded to the GitHub release by the action. |

## Common Usage

```yaml
      - name: Create Release
        uses: ./.github/publish-obs/create-github-release
        with:
          draft: true
          is-prerelease: false
          tag: 15.0.0
          name: My Software 15.0.0
          release-note-file: ${{ format('{0}/release-notes.txt', runner.temp) }}
          files: ${{ format('{0}/*.zip', runner.temp) }}
```

## Notes

* The action is not able to "undraft" a release created as a draft.
* The action is not able to turn a "pre-release" release into a normal release.
* The provided tag name is verified, the action will not create a release with an invalid tag.
* Provided release assets will automatically overwrite existing assets with the same file name.

## Developer Notes

The action uses the `gh` command-line utility to handle all interactions with GitHub releases apart from checking for the validity of the provided tag name and discovering potential release assets by the provided glob expression or JSON array.

* File paths are handled in their UNIX variant and converted from and to Windows format at the input/output edge of the Bash script.

