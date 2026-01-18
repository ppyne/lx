# Examples

Small Lx scripts demonstrating typical use cases.

## calculator.lx

Interactive calculator driven by `read_key`. Supports digits, `.` (and `,`
mapped to `.`), `+`, `-`, `*`, `/`, and `=`. Use `c` to clear and `q` to quit.

## dice_game.lx

Small dice guessing game using `read_key` for single-key input. Prompts
for a number 1-6, tracks wins/losses, and exits on `q`.

## star_trader.lx

Old-school prompt-driven trading game inspired by classic BASIC titles.
Buy/sell goods between planets, manage fuel and cargo, and survive the
occasional pirate encounter.

## lunar_lander.lx

Classic prompt-driven lunar landing game. Manage fuel and throttle to
touch down safely with low impact velocity.

## budget_tracker.lx

Multi-account budget tracker. Record income/expenses, transfer between
accounts, and list recent transactions. Data is saved to a local JSON file.

## http_curl.lx

Implements `http_get`, `http_post`, and `http_request` in Lx by calling
the system `curl` command via `exec()`. Returns `[status, headers, body]`
and accepts optional headers and timeout values.

## sendmail.lx

Sends an email via the local `sendmail` utility using `exec()`. Builds a
simple RFC-822 message with optional extra headers and returns the exit
code.

## imagemagick_resize.lx

Calls the ImageMagick `convert` CLI to resize an image while preserving
aspect ratio, fitting within a bounding box.