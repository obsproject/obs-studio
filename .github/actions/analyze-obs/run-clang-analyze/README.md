# run-clang-analyze Action

The run-clang-analyze action uses Clang Static Analyzer to generate SARIF files which can then be uploaded to GitHub as a CodeQL report after merging the files into a format accepted by GitHub.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|:-------:|
| `architecture` | The CPU architecture used for building OBS Studio. Available values are `x86_64` and `arm64`.| `REQUIRED` |
| `upload-codeql` | A boolean value to indicate whether the generated SARIF report should be automatically uploaded. |`false`|
| `xcode-version` | An Xcode version number to select a specific Xcode version preinstalled on the runner. | `''` |
| `github-token`| The GitHub token required to upload the SARIF file as CodeQL report. The provided token needs to have the `security-events: write` permission. | `github.token`|

### Outputs

The action has no outputs.

## Common Usage

The action requires no prior setup and can be invoked directly on a checkout of the repository:

```yaml
      - name: Analyze OBS Studio
        id: analyze
        uses: ./.github/actions/analyze-obs/run-clang-analyze
        with:
          architecture: arm64
          upload-codeql: true

```

Be aware that just like the `build-obs` action, environment variables can influence project generation by CMake and code paths might not be included in the build and analysis without them. Thus it should be ensured that a "maximalist" build of OBS Studio can be configured to achieve the highest possible coverage of source code.

## Notes

> [!IMPORTANT]
> The action requires a macOS or Linux GitHub Actions runner.

* The CodeQL report uses the category identifier `clang-analyze`.

## Developer Notes

Under the hood the action uses the `build-obs` action with the optional input `analyze` set to `true`. The action thus mostly serves as a convenience wrapper around this action, with the benefit of automatically gathering and merging of all generated SARIF files so that their contents can be (optionally) uploaded as a single CodeQL report.

The merging is handled by the `merge-clang-sarif` action, refer to its documentation for details about the production of a single CodeQL report file.
