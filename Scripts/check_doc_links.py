"""Check every link in the given Markdown docs returns HTTP 200 (after redirects, browser User-Agent).

Usage: python Scripts/check_doc_links.py docs/design/*.md
Prints `<status> <url>` per link; exits 0 only if every link is 200.
"""
import re
import sys
import urllib.error
import urllib.request

USER_AGENT = ("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
              "(KHTML, like Gecko) Chrome/130.0 Safari/537.36")
LINK = re.compile(r"https?://[^\s)\]>`\"']+")


def links(text):
    """Unique http(s) URLs in Markdown text, trailing sentence punctuation dropped."""
    return sorted({u.rstrip(".,;:") for u in LINK.findall(text)})


def status(url):
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    try:
        with urllib.request.urlopen(req, timeout=20) as resp:
            return resp.status
    except urllib.error.HTTPError as e:
        return e.code
    except (urllib.error.URLError, TimeoutError) as e:
        return f"ERR({e})"


def main(paths):
    if not paths:
        print("usage: check_doc_links.py FILE.md ...", file=sys.stderr)
        return 2
    urls = sorted({u for p in paths for u in links(open(p, encoding="utf-8").read())})
    bad = 0
    for url in urls:
        code = status(url)
        print(f"{code} {url}")
        bad += code != 200
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
