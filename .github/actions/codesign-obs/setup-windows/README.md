# setup-windows Action

The setup-windows action sets up Google CNG provider and authenticates with the Google Cloud to create an authenticated environment on a Windows GitHub Actions runner.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `gcp-identity-provider` | The full identifier of the Workload Identity Provider, including the project number, pool name, and provider name. | `REQUIRED` |
| `gcp-account-name` | Email address or unique identifier of the Google Cloud service account. | `REQUIRED` |
| `github-token` | The GitHub token required for the `gh` command-line utility to download the Google CNG tool. | `github.token` |

### Outputs

This action has no outputs.

## Common Usage

The inputs provided to the action should commonly be stored as secrets and need to be provided as inputs to the action.

```yaml
jobs:
  code-sign-windows:
    name: Code Sign Windows Build
    runs-on: windows-2025-vs2026
    environment:
      name: code-signing
      deployment: false
    steps:
      - name: Set Up Code Signing
        uses: ./.github/actions/codesign-obs/setup-windows
        with:
          gcp-identity-provider: ${{ secrets.gcp-identity-string }}
          gcp-account-name: ${{ secrets.gcp-account-name }}
```

## Notes

> [!IMPORTANT]
> The action requires a Windows GitHub Actions runner.

The `gcp-identity-provider` indeed needs to be the full string as documented by Google, e.g.:

```
projects/123456789/locations/global/workloadIdentityPools/my-pool/providers/my-provider
```

The `gcp-account-name` is the email address used for the associated Google account e.g., `my-service-account@my-project.iam.gserviceaccount.com`.

> [!WARNING]
> Secrets should always be stored as environment secrets (and not repository secrets) as this allows a project to require approval by organisation members before an associated workflow actually executes and accesses these secrets.

## Developer Notes

The action automatically downloads and installs Google's Cloud CNG Provider from https://github.com/GoogleCloudPlatform/kms-integrations and runs the https://github.com/google-github-actions/auth action to authenticate with the provided credentials.

This effectively puts the GitHub Actions runner into an "authenticated" state, such that when `signtool.exe` uses the Google CNG provider with the certificate singing request it detects and uses the authentication set up by this action.

* File paths are handled in their UNIX variant and converted from and to Windows format at the input/output edge of the Bash script.
