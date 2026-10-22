"""Give the page script and the runtime files one shared version.

The browser caches traitvis.js and traitvis.wasm separately. A new wasm
with the previous script never starts. The version is the wasm timestamp,
written onto both script addresses after each web link.
"""

import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
version = str((root / "traitvis.wasm").stat().st_mtime_ns)
html_path = root / "traitvis.html"
text = html_path.read_text()
text = re.sub(r'src="upload\.js[^"]*"', f'src="upload.js?v={version}"', text)
text = re.sub(r'src=upload\.js(?=[\s>])', f'src="upload.js?v={version}"', text)
text = re.sub(r'src="traitvis\.js[^"]*"', f'src="traitvis.js?v={version}"', text)
text = re.sub(r'src=traitvis\.js(?=[\s>])', f'src="traitvis.js?v={version}"', text)
html_path.write_text(text)
