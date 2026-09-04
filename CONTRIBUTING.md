# Contributing to Vigil

Thank you for considering contributing to Vigil! ❤️

Whether you are reporting a bug, fixing an issue, proposing an improvement, adding tests or examples, or improving the documentation, your contributions are appreciated.

Please take a moment to read these guidelines before opening an issue or submitting a pull request.

## Reporting Bugs

Please use the [Bug Report](https://github.com/DMsuDev/vigil/issues/new?template=bug_report.yml) template when reporting a bug.

Before opening a new issue, search [open and closed issues](https://github.com/DMsuDev/vigil/issues) to check whether the problem has already been reported or fixed, and verify that it is reproducible with the latest release.

## Suggesting Enhancements

Please use the [Feature Request](https://github.com/DMsuDev/vigil/issues/new?template=feature_request.yml) template when suggesting a new feature or improvement.

Before opening a new issue, search [open and closed issues](https://github.com/DMsuDev/vigil/issues) to check whether the idea has already been suggested.

For significant API changes or architectural proposals, please open an issue first to discuss the approach before starting implementation.

## Submitting Pull Requests

1. **Fork the repository** and create a feature branch off `main`:

   ```bash
   git switch -c feat/my-cool-feature
   ```

2. **Keep changes focused.** Each pull request should address a single bug, feature, refactor, or other clearly defined purpose.
3. **Follow the existing code style.** Match the naming conventions, formatting, and design patterns already used throughout the project.
4. **Ensure clean builds.** Your changes must compile cleanly without introducing new warnings.
5. **Include tests or examples.** Add corresponding unit tests or update example targets under `examples/` when introducing new behavior.
6. **Ensure CI passes.** All relevant GitHub Actions checks should pass before the pull request is merged.

## Examples and Documentation

If a change affects public behavior, APIs, or usage, update the relevant documentation or examples when appropriate.

Examples should remain concise and demonstrate the intended public API rather than internal implementation details.
