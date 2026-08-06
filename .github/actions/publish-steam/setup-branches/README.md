# setup-branches

The setup-branches action selects an appropriate Steam release branch and description based on the GitHub Actions workflow event and git reference.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `ref-name` | The git ref to use for the Steam release. | `github.ref` |
| `github-token` | The GitHub token required for the `gh` command-line utility to check release contents. | `github.token` |

### Outputs

| Output | Description |
|:------:|-------------|
| `branch-name` | The Steam release branch to use for the given git reference. |
| `description` | A descriptive name for the given git reference. |
| `is-prerelease` | A boolean string to indicate whether the provided git reference marks a pre-release version. |

## Common Usage

```yaml
      - name: Setup Branches
        id: setup
        uses: ./.github/actions/publish-steam/setup-branches
        with:
          ref-name: ${{ github.ref_name }}
```

## Notes

The action will attempt to select the appropriate Steam release branch and also provide an appropriate description string based on the GitHub workflow event that it runs for:

* `release` - the default release branch is used, the semantic version is used as description.
* `workflow_dispatch` - uses the beta release branch if dispatched with a pre-release tag, uses the default branch otherwise. The semantic version is used as description.
* `schedule` - uses the nightly release branch and uses the provided ref-name as description.

> [!IMPORTANT]
> The action will fail when dispatched without a git reference that uses a semantic version.

## Developer Notes

The action checks the provided git reference when run in the context of a `workflow_dispatch` to print an appropriate error message, so even if a job-level check is omitted, a non-version tag will make the action fail.
