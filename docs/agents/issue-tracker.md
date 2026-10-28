# Issue tracker: GitHub

Issues and specs for this repository live in GitHub Issues at `jindoridodi/ninajirachi`. Use the `gh` CLI for tracker operations.

## Common operations

```sh
gh repo view --json nameWithOwner,url
gh issue create --title "..." --body "..."
gh issue view <number> --json number,title,body,labels,comments
gh issue list --state open --json number,title,body,labels,comments
gh issue edit <number> --add-label "ready-for-agent"
gh issue edit <number> --remove-label "needs-info"
gh issue comment <number> --body "..."
gh issue close <number>
```

Use GitHub issue dependencies when available. A ticket is ready when all of its blocking issues are closed. If native dependencies are unavailable, record `Blocked by: #<number>` in the issue body.

Before creating issues, confirm the repository with `gh repo view --json nameWithOwner,url`. Create issues only in this repository.

Pull requests are not a triage request surface for this project.

## When a skill says “publish to the issue tracker”

Create or update a GitHub Issue and apply the label required by that skill. Use the project glossary vocabulary in issue titles and bodies.
