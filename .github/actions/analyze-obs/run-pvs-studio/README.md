# run-pvs-studio Action

The run-pvs-studio action uses PVS-Studio to run static code analysis on an existing OBS Studio Visual Studio project and uploads the generated SARIF file as a CodeQL report.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|:-------:|
| `architecture` | The CPU architecture used for building OBS Studio Available values are `x64` and `arm64`. | `REQUIRED` |
| `upload-codeql` | A boolean value to indicate whether the generated SARIF report should be automatically uploaded. |`false`|
| `github-token`| The GitHub token required to upload the SARIF file as CodeQL report. The provided token needs to have the `security-events: write` permission. | `github.token`|

### Outputs

The action has no outputs.

## Common Usage

The action requires no prior setup and can be invoked directly on a checkout of the repository:

```yaml
      - name: Analyze OBS Studio
        id: analyze
        uses: ./.github/actions/analyze-obs/run-pvs-studio
        with:
          architecture: x64
          upload-codeql: true

```

Be aware that just like the `build-obs` action, environment variables can influence project generation by CMake and code paths might not be included in the build and analysis without them. Thus it should be ensured that a "maximalist" build of OBS Studio can be configured to achieve the highest possible coverage of source code.

## Notes

> [!IMPORTANT]
> The action requires a Windows GitHub Actions runner.

* The action does not do any setup or installation of PVS Studio, this needs to be done in preparation before calling the action.
* The generated PVS-Studio log is uploaded as a workflow artifact by default. The CodeQL report is only optionally uploaded.

## Developer Notes

Under the hood the action uses the `build-obs` action to generate a Visual Studio project and builds the project once before passing the generated solution file to PVS Studio. As Microsoft changed the file extension of Visual Studio solution files with Visual Studio 19 2026 to `slnx` the action tries to pick up (and pass along) either variant inside the automatically selected directory path.

* PVS-Studio is capable of converting its own report into a single SARIF tool report by itself, so no merging or fix-up is necessary.
* File paths are handled in their UNIX variant and converted from and to Windows format at the input/output edge of the Bash script.
