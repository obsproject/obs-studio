# check-changes Action

The check-changes action checks for changed files in a git repository based on two git refs, optionally limited by a git-style "diff" filter and a git-style "pathspec" and returns the list of changed files meeting the specified criteria and a boolean flag to use as a conditional value in workflows and actions.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `ref` | A git reference to check for changed files with. | `HEAD` |
| `base` | A git reference to check against. | `''` |
| `filter` | A git-style diff filter string to limit the kinds of changes to check for. | `''` |
| `pathspec` | A git-style "pathspec" string to limit the file paths to check for. | `''` |
| `use-fallback` | A boolean value to indicate whether to use a fallback base reference if the `ref` is invalid. | `true` |
| `working-directory` | The path from which to run the git checks. | `github.workspace` |

### Outputs

| Output | Description |
|:------:|-------------|
| `has-changed-repo-files` | A boolean string to indicate whether any changed files were detected with the given constraints. |
| `changed-repo-files` | A JSON array string of file paths relative to the working-directory of all changed files. |
| `changed-files` | A JSON array string of absolute file paths of all changed files. |

## Common Usage

The main purpose of this action is to allow workflows or actions to abort early or only take some actions if the required kind of changes have been detected for the specified files based on the commit history between the provided git reference and the base reference.

If no changed files have been detected by the action, the repository (or working-directory) can be considered "clean" and jobs or steps that should only run on changed files can be skipped.

```yaml
      - name: Check for Changed Files
        id: checks
        uses: ./.github/actions/check-changes
        with:
          filter: 'ACM'
          pathspec: '*.c *.h *some_directory/**/*.txt :!excluded_directory/*'

      - name: Handle Result
        shell: bash
        env:
          HAS_CHANGED: ${{ steps.checks.outputs.has-changed-files }}
        run: |
          if [[ "${HAS_CHANGED:-false}" == 'false' ]]; then
            echo "::notice::No necessary file changes detected".
          fi

      - name: Continue Jobs
        if: ${{ fromJSON(steps.checks.outputs.has-changed-files) }}
        ...
```

## Notes

The syntax of both the diff filter as well as the pathspec is available in the git documentation. The provided values are passed directly to `git` and should support all expressions that can also be used on the command line.

A typical diff filter value is `ACM`, which limits the changes to added, created, and modified files. Pathspecs can use both inclusions as well as exclusions (using the `:!<pathspec>` syntax) which allows the action to also ignore some changes and consider the repository "clean".

### Default Base Git Refs

The default git reference to check from is the `HEAD` ref, which is an alias for the most recent commit on the checked out branch, thus this alias depends on the checkout of the git repository in which the action is run.

If a base reference is provided, but it cannot be resolved in the git checkout (either because it doesn't exist in the commit history or the checkout is incomplete), the git reference of the `null` tree is used by default. This will result in the entire history of the checkout to be used and can be prohibited by setting `use-fallback` to `false`, which will then make the action fail instead.

If no base reference is provided, the action will attempt to use different values based on the event type and the state of the checkout:

* By default the `HEAD~1` alias is used, which should point to the commit before the current `HEAD` commit. This requires a checkout depth of at least `2`.
* If a GitHub `pull_request` event is detected, the `HEAD` commit of the base branch targeted by the pull request is used.
* If a GitHub `push` request is detected and the event is not the result of a force-push, use the reference provided by the event's `before` property. which potentially provides the SHA of the most recent commit on the same branch before the current push.
  * If this SHA is invalid, the `null` tree is used instead.

## Developer Notes

The action is a convenience wrapper around `git diff`, composing a corresponding invocation of the command-line tool and also parsing the output into corresponding JSON objects for use with other workflows or actions.

The git ref `4b825dc642cb6eb9a060e54bf8d69288fbee4904` is a "hidden" SHA hash of the "empty tree" (which the action yields programmatically by hashing the `/dev/null` tree). This SHA will always exist and is used as a last-resort fallback (if not disabled by the user) to ensure that the invocation of `git diff` always gets at least one valid git ref.

* Absolute file paths are converted into Windows paths on Windows GitHub Actions runners.
