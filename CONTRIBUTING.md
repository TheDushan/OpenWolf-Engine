# Contributing

Thanks for contributing to this repository!

This repository follows the following conventions:

* [Semantic Versioning](https://semver.org/)
* [Keep a Changelog](https://keepachangelog.com/)
* [Conventional Commits](https://www.conventionalcommits.org/)

To contribute a change:

1. Create a fork of the existing repository
    1. On the project’s home page, in the top right, click Fork.
    1. Below *Select a namespace to fork the project*, identify the project you want to fork to, and click Select. Only namespaces you have Developer and higher permissions for are shown.
    1. GitHub creates your fork, and redirects you to the project page for your new fork. The permissions you have in the namespace are your permissions in the fork.
1. Make the changes in code.
1. Make commits using the [Conventional Commits](https://www.conventionalcommits.org/) format. This helps with automation for changelog. Update `CHANGELOG.md` in the same commit using the [Keep a Changelog](https://keepachangelog.com). Depending on tooling maturity, this step may be automated.
1. Ensure all new commits from the `master` branch are rebased into your branch.
1. Open a merge request. If this merge request is solving a preexisting issue, add the issue reference into the description of the MR.
1. Wait for a maintainer of the repository (see CODEOWNERS) to approve.
1. If you have permissions to merge, you are responsible for merging. Otherwise, a CODEOWNER will merge the commit.
