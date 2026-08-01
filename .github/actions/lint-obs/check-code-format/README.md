# check-code-format

The check-code-format action allows a workflow to run an available linter on the specified set of files. If desired, the action will fail on the very first file that fails linting, or alternatively fails after all specified files have been linted and any single one resulted in failure.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `linter` | The linter to run with the specified files. Available linters are `clang-format`, `gersemi`, `swift-format`, `zizmor`, and `xmllint`. | `REQUIRED` |
| `files` | A JSON string representing an array of file paths for files to lint. On macOS and Linux GitHub Actions runners this also supports glob patterns. | `REQUIRED`|
| `fail-on` | A string indicating the fail condition to use by the action. Available values are `never`, `fast`, and `error`. | `never` |
| `working-directory` | The path to an OBS Studio checkout. This is necessary to find the formatter scripts in the `build-aux` directory. | `github.workspace` |

### Outputs

This action has no outputs.

## Common Usage

The action is designed to handle the output of the `check-changes` action, so that they can be used in combination with each other.

```yaml
      - name: Check for Changed Files
        id: checks
        uses: ./.github/actions/check-changes
        with:
          filter: 'ACM'
          pathspec: '*.c *.h *.cpp *.hpp *.m *.mm'

      - name: Run clang-format
        if: fromJSON(steps.checks.outputs.has-changed-files)
        uses: ./.github/actions/lint-obs/check-code-format
        with:
          linter: clang-format
          files: ${{ steps.checks.outputs.changed-repo-files }}
          fail-on: error
```

## Notes

> [!IMPORTANT]
> On macOS and Linux GitHub Actions runners all linters are supported. On Windows only `clang-format`, `gersemi`, and `zizmor` are supported.

The result of linting is indicated by the result of the action itself. If all files pass linting, the action will succeed without error. If `fail-on` is set to anything but `never`, any file that fails linting will make the action fail. The major difference is in _when_ the action will fail:

* When `fast` is chosen the action will fail and abort on the _first_ file that fails linting.
* When `error` is chosen all specified files will be run through the linter, but the action will ultimately fail if any single file failed linting.

The action also invokes the linters with the option to use GitHub-style output, which will report failures as annotations.

## Developer Notes

The action makes use of the formatting launcher scripts available in OBS Studio's `build-aux` directory and are thus not entirely standalone.

The version of `clang-format` used by linters is locked to the version available with the canonical Visual Studio version expected by the project. This was done for the simple reason that it's the most common way for Windows developers to get access to `clang-format` (which cannot be as easily installed on Windows as on other platforms).

> [!NOTE]
> This action uses Powershell scripts on GitHub Actions Windows runners to match the script language used by the launcher scripts in the `build-aux` directory.
