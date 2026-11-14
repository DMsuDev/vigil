# Contributing to Vigil

Thank you for your interest in contributing to Vigil. Bug reports, feature proposals, and project improvements are welcome.

Please review these guidelines before opening an issue or submitting a pull request.

## Reporting Bugs

Please use the [Bug Report](https://github.com/DMsuDev/vigil/issues/new?template=bug_report.yml) template when reporting a bug.

Before opening a new issue:

- Search [existing issues](https://github.com/DMsuDev/vigil/issues) to avoid duplicates.
- Verify that the issue is reproducible with the latest release.
- Include clear reproduction steps and relevant environment details.

## Suggesting Enhancements

Use the [Feature Request](https://github.com/DMsuDev/vigil/issues/new?template=feature_request.yml) template when proposing a new feature or improvement.

Before submitting a proposal, search [existing issues](https://github.com/DMsuDev/vigil/issues) to check whether the idea has already been discussed.

For significant API changes or architectural proposals, please open an issue first to discuss the approach before starting implementation.

## Submitting Pull Requests

1. Fork the repository and create a feature branch off `main`:

   ```bash
   git switch -c feat/my-cool-feature
   ```

2. Keep changes focused. Each pull request should address a single bug, feature, refactor, or other clearly defined purpose.
3. Follow the existing code style. Match the naming conventions, formatting, and design patterns used throughout the project.
4. Ensure clean builds. Changes must compile without introducing new warnings.
5. Include tests. Add or update unit tests to cover new behavior and prevent regressions. Update examples when they help demonstrate changes to the public API.
6. Update relevant documentation and examples when changes affect public APIs, behavior, or usage.
7. Follow [Conventional Commits](https://www.conventionalcommits.org) when writing commit messages; see the [commit guidance](docs/development.md#commits-and-changelog).
8. Ensure CI passes. All relevant GitHub Actions checks must pass before a pull request is merged.

## Examples and Documentation

When changes affect public APIs, behavior, or usage, update the relevant documentation and examples to keep them consistent with the implementation.

Examples should remain concise and demonstrate the intended public API rather than internal implementation details. Add or update an example when it provides value to users.

## Development Workflow

For build instructions, testing, code style, commit conventions, and release procedures, see the [Development Guide](docs/development.md).

For an overview of Vigil's internal structure and design decisions, see the [Architecture Guide](docs/architecture.md).
