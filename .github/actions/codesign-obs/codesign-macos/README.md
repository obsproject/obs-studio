# codesign-macos Action

The codesign-macos action code signs and optionally notarizes an existing OBS Studio disk image.

## Documentation

### Inputs

| Input | Description | Default |
|:-----:|-------------|---------|
| `path` | A path to an OBS Studio disk image file present on the GitHub Actions runner. | `REQUIRED` |
| `identity` | The Apple Developer ID to code sign the disk image with. | `REQUIRED` |
| `team` | The Apple Developer team ID to code sign the disk image with. | `REQUIRED` |
| `notarize` | A boolean value to indicate whether the disk image should also be notarized. | `false` |
| `notarization-user` | The Apple ID used to authenticate with Apple's notarization servers. | `''` |
| `notarization-password` | The app password to authenticate with Apple's notarization servers. | `''` |

### Outputs

| Output | Description |
|:------:|-------------|
| `path` | The path to the code signed and optionally notarized OBS Studio disk image. |

## Common Usage

The action requires the corresponding Apple Developer certificate the be installed on the macOS GitHub Actions runner before calling this action.

```yaml
      - name: Set Up Code Signing
        ...

      - name: Code Sign macOS Disk Image
        id: codesign
        uses: ./.github/actions/codesign-obs/codesign-macos
        with:
          path: ${{ format('{0}/obs-studio-arm64.dmg', runner.temp) }}
          identity: ${{ secrets.macos-developer-id }}
          team: ${{ secrets.macos-developer-team }}
          notarize: true
          notarization-user: ${{ secrets.macos-apple-id }}
          notarization-password: ${{ secrets.macos-apple-password }}
```

Do not rely on the action to code sign and notarize the disk image in place, use the `path` output instead to unambiguously identify the output disk image on the GitHub Actions runner.

## Notes

> [!IMPORTANT]
> The action requires a macOS GitHub Actions runner.

The `codesign-obs/setup-macos` action can be used to set up an Apple Developer certificate on the GitHub Actions runner before running this action.

While it is not necessary to provide an actual Apple Developer ID and team ID (providing just a dash `-` as the Developer ID and an empty string as the team ID is sufficient), the disk image will only receive an ad-hoc signature that is valid outside of the GitHub Actions runner.

Thus there is no benefit for distribution between an unsigned disk image and a disk image signed with an ad-hoc profile.

## Developer Notes

The action uses `codesign` and `xcrun notarytool` to handle both code signing and notarization. The first requires an Apple Developer certificate matching the provided developer ID to be installed in the GitHub Actions runner's key-chain, while the former can store and use the credentials directly.
