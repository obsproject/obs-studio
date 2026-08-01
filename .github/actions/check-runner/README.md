# check-runner Action

The check-runner action checks the current runner against the set of implemented conditions.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `os` | A multiline string of runner operating system strings. | `macOS Windows Linux` |
| `custom-error` | A custom error message to use when runner fails checks. | `''` |

### Outputs

The action has no output.

## Common Usage

```yaml
      - name: Checkout
        uses: actions/checkout@9c091bb21b7c1c1d1991bb908d89e4e9dddfe3e0 # v7.0.0
        with:
          persist-credentials: false
          fetch-depth: 1

      - name: Check Runner Operating System
        uses: ./.github/actions/check-runner
        with:
          os: |
            Windows
            macOS
```

## Notes

* Additional checks of the runner environment required by another action or workflow should be implemented in this action and selected/applied when corresponding inputs are provided.
