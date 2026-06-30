# generate-docs

The generate-docs action installs the necessary toolchain to run generate `Sphinx`-based documentation and uploads the generated files as a workflow artifact.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `disable-link-extensions` | A boolean value to indicate whether the generated documentation should not use links with `.html` file extensions. | `false` |
| `working-directory` | The path to an OBS Studio checkout's root directory. | `github.workspace` |

### Outputs

| Output | Description |
|:------:|-------------|
| `artifact-name` | The name of the workflow artifact uploaded by the action. |

## Common Usage

The action can be run on a checkout of OBS Studio, as it requires the contents of the `docs` directory. If the checkout was placed in a differnt directory but the GitHub workspace, the path can be provided via the `working-directory` input.

```yaml
      - name: Generate Documentation
        id: generate
        uses: ./.github/actions/generate-docs
        with:
          disable-link-extensions: true
```

## Notes

> [!IMPORTANT]
> The action requires a macOS or Linux GitHub Actions runner.

* Disabling link extensions is mainly used for deployments of the documentation to CloudFlare pages. For local browsing of the documentation link extensions should be retained.

## Developer Notes

The action will automatically install or update a version of Python 3 on the GitHub Actions runner and use `pip` to install the necessary projects (including `sphinx` and `poetry`) to build the documentation.

Before the build process can be started, the Sphinx configuration file needs to be updated, which the action automatically takes care of:

* The `libobs` ABI version is encoded in the `obs-config.h` header. This version needs to be pushed into the `Sphinx` configuration file in the `docs` directory.
* The copyright string is also updated using the current year at every invocation of the action.

The artifact name then differes based on whether link extensions are enabled or not. This helps with disambiguating generated documentation if the action is called twice within the same workflow job, once with link extensions disabled and once without.
