# check-version-tag Action

The check-version-tag action checks whether the name of the provided git ref represents a semantic version string. If possible, the version segments are extracted from a valid version string and the action can also optionally fail if no valid version string is detected.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `ref` | A git reference to check for a semantic version string. | `REQUIRED` |
| `fail-on-mismatch` | A boolean value to indicate whether the action should fail if no semantic version was detected. | `false` |

### Outputs

| Output | Description |
|:------:|-------------|
| `version` | A string representing the full version. |
| `major` | A string representing the major version segment. |
| `minor` | A string representing the minor version segment. |
| `patch` | A string representing the patch version segment. |
| `pre-release` | A string representing the pre-release version segment (e.g. `-rc2`). |
| `number` | A string representing just the pre-release number (e.g. `2`).|
| `is-pre-release` | A boolean string representing whether or not the git ref is a valid pre-release semantic version. |
| `is-valid-semver` | A boolean string representing whether or not the git ref is a valid semantic version. |

## Common Usage

This action can commonly be used to check if the tag name of a pushed tag actually represents a semantic version (e.g. `5.0.0-beta2`) and allows a workflow to either abort or change its behavior. This also allows GitHub Actions to differentiate between a pushed tag for a release or a pushed tag for any other purpose.

This also allows dispatched workflows to abort early if they have been triggered with a git ref that does not represent a semantic version string.

```yaml
      - name: Checkout
        uses: actions/checkout@9c091bb21b7c1c1d1991bb908d89e4e9dddfe3e0 # v7.0.0
        with:
          persist-credentials: false
          fetch-depth: 1

      - name: Generate Semver From Tag
        id: semver
        uses: ./.github/actions/check-version-tag
        with:
          ref: ${{ github.ref_name }}
          fail-on-mismatch: true
```

## Notes

The action only checks the git ref's name, but not the _type_ of the git ref. Thus the action will yield outputs (and succeed) if the git ref represents a branch named `2.5.0` and not only a tag.

To ensure that a workflow only runs on a tag and that tag uses a semantic version name, the `github.ref_type` value needs to be combined with the result of this action.

## Developer Notes

The action runs a simple regular expression on the provided git ref to yield matches for a common semantic version string of the form `<1-n digits>.<1-n digits>.<1-n digits>-<rc or beta><1-n digits>`. There is no further processing beyond that. If Bash's regular expression matching fails, the git ref is not considered a semantic version string.
