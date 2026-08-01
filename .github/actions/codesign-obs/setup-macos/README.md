# setup-macos Action

The setup-macos action sets up an Apple Developer certificate in the key-chain of a macOS GitHub Actions runner for use with code signing and also sets up a provisioning profile required for system extensions support used by OBS Studio.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `identity` | The Apple Developer ID associated with the developer certificate that will be installed in the GitHub Actions runner's key-chain. | (Required) |
| `cert` | The Apple Developer PKCS12 certificate as a base64-encoded string. | (Required) |
| `cert-password` | The password required to unlock the PKCS12 certificate. | (Required) |
| `provisioning-profile` | The provisioning profile as a base64-encoded string. | `''` |

### Outputs

| Output | Description |
|:------:|-------------|
| `can-codesign` | A boolean string to indicate whether the provided inputs enable code signing on the GitHub Actions runner. |
| `identity` | The Apple Developer ID for which a code signing certificate was successfully installed. |
| `team` | The Apple Developer team for which a code signing certificate was successfully installed. |
| `profile` | A boolean string to indicate whether a provisioning profile was installed on the GitHub Actions runner. |
| `profile-uuid` | The UUID of the provisioning profile that was installed on the GitHub Actions runner. |

## Common Usage

The inputs provided to the action should commonly be stored as secrets and need to be provided as inputs to the action.

```yaml
jobs:
  code-sign-macos:
    name: Code Sign macOS Disk Image
    runs-on: macos-26
    environment:
      name: code-signing
      deployment: false
    steps:
      - name: Set Up Code Signing
        id: setup
        uses: ./.github/actions/codesign-obs/setup-macos
        with:
          identity: ${{ secrets.macos-developer-id }}
          cert: ${{ secrets.macos-developer-cert }}
          cert-password: ${{ secrets.macos-developer-cert-pass }}
```

## Notes

> [!IMPORTANT]
> The action requires a macOS GitHub Actions runner.

Code signing certificates need to be installed in the macOS key-chain to be available for `codesign` and identified by the Apple Developer team (or Apple Developer ID). By default macOS will also require a password when exporting the developer certificate and key, which should use a random password that is only used by the GitHub Actions runner.

> [!WARNING]
> Secrets should always be stored as environment secrets (and not repository secrets) as this allows a project to require approval by organization members before an associated workflow actually executes and accesses these secrets.

## Developer Notes

As the certificate is a base64-encoded string, the value will be decoded first before being dumped into a temporary file on the GitHub Actions runner's disk and imported into a new temporary key-chain. This key-chain is then made available for Apple's command-line tools which avoids having to interact with the system key-chain.

The temporary key-chain uses a random password but is also unlocked automatically by the action and thus is not shared with the workflow.
