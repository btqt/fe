"""Parse a digest text file and recreate the files it describes on disk.

The digest format uses blocks delimited by a line of '=' characters, a
"FILE: <path>" header line, another '=' delimiter line, then the file
content up to the next delimiter block (or end of file):

================================================
FILE: path/to/file/A
================================================
<content of file A>
"""
import argparse
import sys
from pathlib import Path

DELIMITER_PREFIX = "="
FILE_HEADER_PREFIX = "FILE:"


def is_delimiter(line: str) -> bool:
    """Return True if the line is a delimiter line made only of '=' characters."""
    stripped = line.strip()
    return len(stripped) >= 10 and set(stripped) == {DELIMITER_PREFIX}


def parse_digest(text: str) -> list[tuple[str, str]]:
    """Split digest text into a list of (relative_path, content) pairs."""
    lines = text.splitlines()
    entries: list[tuple[str, str]] = []
    i = 0
    n = len(lines)
    while i < n:
        if not is_delimiter(lines[i]):
            i += 1
            continue
        header_line = lines[i + 1] if i + 1 < n else ""
        if not header_line.startswith(FILE_HEADER_PREFIX):
            i += 1
            continue
        if i + 2 >= n or not is_delimiter(lines[i + 2]):
            i += 1
            continue

        file_path = header_line[len(FILE_HEADER_PREFIX):].strip()
        content_start = i + 3
        content_end = content_start
        while content_end < n and not is_delimiter(lines[content_end]):
            content_end += 1

        content_lines = lines[content_start:content_end]
        while content_lines and content_lines[-1] == "":
            content_lines.pop()

        entries.append((file_path, "\n".join(content_lines)))
        i = content_end
    return entries


def write_files(entries: list[tuple[str, str]], output_root: Path) -> int:
    """Create each file (with parent directories) and write its content. Returns count written."""
    count = 0
    for rel_path, content in entries:
        target = output_root / rel_path
        target.parent.mkdir(parents=True, exist_ok=True)
        body = f"{content}\n" if content else ""
        target.write_text(body, encoding="utf-8")
        print(f"Written: {target}")
        count += 1
    return count


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Recreate files described in a digest text file."
    )
    parser.add_argument("digest_path", help="Path to the digest text file.")
    parser.add_argument(
        "-o", "--output-root",
        default="remotediag",
        help="Root directory to write files into (default: %(default)s).",
    )
    args = parser.parse_args()

    digest_path = Path(args.digest_path)
    if not digest_path.is_file():
        print(f"Digest file not found: {digest_path}", file=sys.stderr)
        sys.exit(1)

    text = digest_path.read_text(encoding="utf-8", errors="replace")
    entries = parse_digest(text)
    if not entries:
        print("No file entries found in digest.", file=sys.stderr)
        sys.exit(1)

    output_root = Path(args.output_root)
    written = write_files(entries, output_root)
    print(f"Done. {written} file(s) written under {output_root.resolve()}")


if __name__ == "__main__":
    main()
