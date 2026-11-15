"""Download the CC0 trap models listed in trap_fit.THIRDPARTY into models/thirdparty/traps/.
    python models/fetch_traps.py      (engine-free; commit the files, CREDITS.md already names them)
"""
import os
import urllib.request

import trap_fit


def main():
    os.makedirs(trap_fit.THIRDPARTY_DIR, exist_ok=True)
    for name, (fname, url, _colour) in trap_fit.THIRDPARTY.items():
        dest = os.path.join(trap_fit.THIRDPARTY_DIR, fname)
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req, timeout=30) as r, open(dest, 'wb') as fh:
            fh.write(r.read())
        print(name, '->', dest, os.path.getsize(dest), 'bytes')


if __name__ == '__main__':
    main()
