# validate-json-schema

The validate-json-schema action runs schema checks on the provided list of JSON files and annotates them if any schema violations were detected.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `path` | A string representing either a JSON array of files, a glob expression, or a relative file path. | `REQUIRED` |
| `github-token`| The GitHub token required to post schema validation annotations. The provided token needs to have the `checks: write` permission. | `github.token`|
| `working-directory` | A path to the root directory to base relative file paths on. | `github.workspace` |

### Outputs

This action has no outputs.

## Common Usage

The action is designed to handle the output of the `check-changes` action, so that they can be used in combination with each other, but can also optionally handle glob expressions directly.

```yaml
      - name: Check for Changed Files
        id: checks
        uses: ./.github/actions/check-changes
        with:
          pathspec: '**/*.json'

      - name: Run JSON Validation
        if: fromJSON(steps.checks.outputs.has-changed-files)
        uses: ./.github/actions/lint-obs/validate-json-schema
        with:
          path: ${{ steps.checks.outputs.changed-repo-files }}
          github-token: ${{ github.token }}

      ...

      - name: Run JSON Validation
        if: fromJSON(steps.checks.outputs.has-changed-files)
        uses: ./.github/actions/lint-obs/validate-json-schema
        with:
          path: ${{ format('{0}/some_path/*.json', github.workspace) }}
          github-token: ${{ github.token }}
```

## Notes

> [!IMPORTANT]
> The action requires a macOS or Linux GitHub Actions runner.

## Developer Notes

The action uses a custom Python script to run actual schema validation, but will also use appropriate outputs if a GitHub Actions environment is detected. The script itself requires the https://github.com/python-jsonschema/jsonschema and https://github.com/open-alchemy/json-source-map/wiki Python modules.
