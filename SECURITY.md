# Aerium Security Policy

Aerium is an independent, early-stage fork of OBS Studio. This policy covers Aerium's repository and changes; it is not the OBS Project's security policy.

## Reporting a Vulnerability

Use [GitHub private vulnerability reporting](https://github.com/AeriumChris/Aerium/security/advisories/new) to contact the Aerium maintainer privately. Do not disclose vulnerabilities, credentials, personal data, or private recordings in public issues or pull requests.

Include, where available:

- The affected commit or build and operating system.
- The expected behavior, observed behavior, and security impact.
- Minimal reproduction steps using synthetic data.
- Relevant logs with credentials and personal information removed.
- Whether the issue also reproduces in an official OBS Studio release.

For an issue that also affects upstream OBS Studio, follow the [OBS Project's security policy](https://github.com/obsproject/obs-studio/security/policy). Reporting to Aerium does not automatically notify OBS; please coordinate disclosure with all affected maintainers.

For a dependency vulnerability, also use the dependency project's reporting process where appropriate. Do not test third-party services or infrastructure without their authorization.

## Supported Versions

There are no supported public Aerium releases yet. Security work currently targets the latest development branch, `master`. Older commits and unofficial builds do not have a security maintenance guarantee.

Development builds are experimental and should not be treated as production-ready. They are not distributed through OBS's signing or update infrastructure.

## Handling Reports

Reports are reviewed on a best-effort basis. As a small early-stage project, Aerium cannot promise a response deadline, fix date, bounty payment, or legal safe harbor. Please allow time for investigation and coordinate public disclosure through the private report.

Only use systems and data you are authorized to test. Minimize data access, avoid disruption, and stop testing if you encounter private information. Never include secrets in AI prompts or public diagnostic output.

## Other Reports

Use [Aerium Issues](https://github.com/AeriumChris/Aerium/issues) for non-security bugs and feature requests. For abuse or conduct concerns, see the [Code of Conduct](COC.rst).
