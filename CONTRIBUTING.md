# Contributing

Team branches are set up so everyone can work in parallel without pushing directly to `main`.

## Branches

| Person | Branch   | Use for                          |
| ------ | -------- | -------------------------------- |
| Sam    | `sam`    | Sam's work and experiments       |
| Nhat   | `nhat`   | Nhat's work and experiments      |
| Darren | `darren` | Darren's work and experiments    |
| Rajiv  | `main`   | Integration only, via pull requests |

## Workflow

1. Clone the repo and run the one-time setup:

   ```bash
   ./scripts/setup-git.sh
   ```

2. Check out your branch:

   ```bash
   git checkout sam    # or nhat / darren
   ```

3. Make changes, commit, and push to your branch:

   ```bash
   git add .
   git commit -m "Describe your change"
   git push
   ```

4. Open a pull request from your branch into `main` on GitHub.

5. Wait for the **Compile examples** check to pass.

6. Get one review, then merge the pull request.

## Rules for `main`

- No direct pushes to `main` (blocked locally and on GitHub).
- All changes reach `main` through pull requests.
- Pull requests must pass CI before merge.

## First-time repo owner setup

After cloning on a new machine:

```bash
brew install gh          # if needed
gh auth login
./scripts/setup-git.sh
./scripts/protect-main.sh
```
