# create-windows-installer

The create-windows-installer action creates and code signs an NSIS-based installation program for Windows platforms. It also strips and separates all program database (`.pdb`) files and also produces a new compressed archive of the build.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `path` | The path to a compressed archive containing an OBS Studio build. | `REQUIRED` |
| `architecture` | The build architecture to generate an NSIS-based installation program for. | `REQUIRED` |
| `version` | The version string to generate an NSIS-based installation program for. | `REQUIRED` |

### Outputs

| Output | Description |
|:------:|-------------|
| `installer-path` | The path to the generated NSIS-based installation program. |
| `pdb-path` | The path to the generated compressed archive of the stripped program database files. |
| `archive-path` | The path to the generated compressed archive of the OBS Studio build. |

## Common Usage

The action requires Windows code signing credentials to be set up on the GitHub Actions runner and thus should be used in conjunction with the `codesign-obs/setup-windows` action.

```yaml
      - name: Set Up Code Signing
        uses: ./.github/actions/codesign-obs/setup-windows
        with:
          gcp-identity-provider: ${{ secrets.gcp-identity-string }}
          gcp-account-name: ${{ secrets.gcp-account-name }}

      - name: Create Windows Installer
        id: installer
        uses: ./.github/actions/publish-obs/create-windows-installer
        with:
          path: ${{ format('{0}/builds', runner.temp) }}
```

## Notes

> [!IMPORTANT]
> The action requires a Windows GitHub Actions runner.

* Running the action with the  `arm64` architecture is unsupported because the creation script depends on a custom DLL with NSIS plugins that is not available for any architectures but `x64` at the moment.
* The compressed archive of the OBS Studio build to generate an installation program for needs to be download separately.
* The action will _not_ code sign the provided build, it will only code sign the installation program.

## Developer Notes

The action uses https://github.com/obsproject/bouf to generate the installation program and archives. BOUF and its dependencies are automatically installed on the GitHub Actions runner by the action.

The setup requirements are extracted from an OBS Studio `CMakePresets.json` file using the following `jq` query:

```typescript
  .configurePresets[]
  // Select the preset with the "dependencies" name
  | select(.name == "dependencies")
  // Select the "bouf" key from the "tools" object
  | .vendor["obsproject.com/obs-studio"].tools.bouf
  // Convert each {"some_key": "some_value"} pair into an array of
  // {"key": "some_key", "value": "some_value"} elements
  | to_entries
  // Convert each array element into a "some_key=some_value" string.
  | map("\(.key)=\(.value|tostring)")
  // Print each array item as separate output line for use as GitHub Actions output.
  | .[]
```

This allows `jq` to output key-value pairs directly in the format required by GitHub Actions.

* BOUF requires all binary dependencies to be installed regardless of which functionality is used, so NSIS and Pandoc need to always be installed.
  * The Pandoc version provided by `winget` installs into the local application data directory, the exact location is environment-dependant. So no fixed location can be provided in BOUF's config file and instead the `PATH` needs to be used.
* File paths are handled in their UNIX variant and converted from and to Windows format at the input/output edge of the Bash script.
