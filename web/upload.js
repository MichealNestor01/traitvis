// Folder drop and the file picker. The config is parsed in C++; this file only copies bytes.
(function () {
  const sliceBytes = 8 << 20;

  function status(message) {
    document.getElementById('status').textContent = message;
  }

  function showError(message) {
    document.getElementById('error').textContent = message;
  }

  function setProgress(ratio) {
    const bar = document.getElementById('progress');
    if (ratio <= 0 || ratio >= 1) {
      bar.hidden = true;
      bar.value = 0;
      return;
    }
    bar.hidden = false;
    bar.value = ratio;
  }

  function setBusy(busy) {
    document.getElementById('folder').disabled = busy || !window.traitvisRuntimeReady;
    document.getElementById('drop').setAttribute('aria-disabled', busy || !window.traitvisRuntimeReady ? 'true' : 'false');
  }

  function parseListing(listing) {
    const attributes = [];
    for (const line of listing.split('\n')) {
      if (!line) continue;
      const parts = line.split('\t');
      if (parts.length < 3) throw new Error('The dataset listing was not readable.');
      attributes.push({ file: parts[0], name: parts[1], count: Number(parts[2]) });
    }
    return attributes;
  }

  function readCString(ptr) {
    const bytes = [];
    const heap = Module.HEAPU8;
    for (let index = ptr; heap[index] !== 0; index++) bytes.push(heap[index]);
    return new TextDecoder().decode(new Uint8Array(bytes));
  }

  function ensureDatasetDir() {
    try {
      Module.FS.mkdir('/dataset');
    } catch (error) {
      // The directory is already there from an earlier drop.
    }
  }

  async function copyInSlices(file, ptr, onProgress) {
    for (let offset = 0; offset < file.size; offset += sliceBytes) {
      const end = Math.min(file.size, offset + sliceBytes);
      const bytes = new Uint8Array(await file.slice(offset, end).arrayBuffer());
      // Re-read the heap view. An earlier allocation can replace it.
      Module.HEAPU8.set(bytes, ptr + offset);
      onProgress(end / file.size);
    }
  }

  function readEntries(reader) {
    return new Promise((resolve, reject) => {
      const entries = [];
      const readBatch = () => {
        reader.readEntries(batch => {
          if (!batch.length) resolve(entries);
          else {
            entries.push(...batch);
            readBatch();
          }
        }, reject);
      };
      readBatch();
    });
  }

  async function filesFromEntry(entry) {
    if (entry.isFile) {
      const file = await new Promise((resolve, reject) => entry.file(resolve, reject));
      return [file];
    }
    const children = await readEntries(entry.createReader());
    const files = [];
    for (const child of children) files.push(...await filesFromEntry(child));
    return files;
  }

  async function loadFolder(fileList) {
    const files = Array.from(fileList);
    const configFile = files.find(file => file.name === 'config.txt');
    if (!configFile) {
      showError('The folder needs a config.txt.');
      return;
    }

    setBusy(true);
    showError('');
    status('Reading config…');
    try {
      ensureDatasetDir();
      Module.FS.writeFile('/dataset/config.txt', new Uint8Array(await configFile.arrayBuffer()));
      const listingPtr = Module.ccall('traitvisDescribeConfig', 'number', ['string'], ['/dataset/config.txt']);
      if (!listingPtr) throw new Error('Could not read the dataset config.');
      const listing = readCString(listingPtr);
      Module._free(listingPtr);
      if (listing.startsWith('!')) throw new Error(listing.slice(1).trim());

      const attributes = parseListing(listing);
      if (!attributes.length) throw new Error('The config has no attributes.');

      let copied = 0;
      const total = attributes.reduce((sum, attribute) => sum + attribute.count * 4, 0);
      for (const attribute of attributes) {
        const file = files.find(item => item.name === attribute.file);
        if (!file) throw new Error('Missing attribute file ' + attribute.file + '.');
        if (file.size !== attribute.count * 4) {
          throw new Error(attribute.file + ' is not the size the config describes.');
        }
        status('Copying ' + attribute.name + '…');
        const dest = Module.ccall(
          'traitvisBeginAttribute', 'number', ['string', 'number'], [attribute.file, attribute.count]) >>> 0;
        if (!dest) throw new Error('Could not allocate ' + attribute.name + '.');
        await copyInSlices(file, dest, ratio => setProgress((copied + ratio * file.size) / total));
        copied += file.size;
      }

      const started = Module.ccall(
        'traitvisBeginLoad', 'number', ['string', 'string'], ['/dataset/config.txt', '']);
      if (!started) throw new Error('A dataset is already loading.');
      const noun = attributes.length === 1 ? 'attribute' : 'attributes';
      status('Loaded ' + attributes.length + ' ' + noun + '.');
    } catch (error) {
      Module.ccall('traitvisCancelPending', null, [], []);
      status('');
      showError(error && error.message ? error.message : String(error));
    } finally {
      setBusy(false);
      setProgress(0);
    }
  }

  window.traitvisLoadFolder = loadFolder;

  function bind() {
    const folder = document.getElementById('folder');
    const drop = document.getElementById('drop');
    folder.addEventListener('change', () => {
      if (folder.files && folder.files.length) loadFolder(folder.files);
      folder.value = '';
    });
    drop.addEventListener('dragover', event => {
      event.preventDefault();
      if (window.traitvisRuntimeReady) drop.classList.add('drag');
    });
    drop.addEventListener('dragleave', () => drop.classList.remove('drag'));
    drop.addEventListener('drop', async event => {
      event.preventDefault();
      drop.classList.remove('drag');
      if (!window.traitvisRuntimeReady) return;
      const collected = [];
      const items = event.dataTransfer.items;
      if (items && items.length && items[0].webkitGetAsEntry) {
        for (const item of items) {
          const entry = item.webkitGetAsEntry();
          if (entry) collected.push(...await filesFromEntry(entry));
        }
      } else {
        collected.push(...event.dataTransfer.files);
      }
      if (collected.length) loadFolder(collected);
    });
    setBusy(false);
  }

  window.traitvisUiReady = function () {
    window.traitvisRuntimeReady = true;
    setBusy(false);
    status('Drop a dataset folder, or choose one.');
  };

  const canvas = document.getElementById('canvas');
  const fallback = document.getElementById('webgl-fallback');
  // The shell minifier drops braces in an inline script, so the runtime object lives here.
  window.Module = {
    canvas: canvas,
    onRuntimeInitialized: function () { traitvisUiReady(); },
    print: function () { console.log.apply(console, arguments); },
    printErr: function () {
      console.error.apply(console, arguments);
      const text = Array.prototype.join.call(arguments, ' ');
      if (text.indexOf('WebGL') !== -1) {
        canvas.style.display = 'none';
        fallback.style.display = 'block';
      }
    }
  };

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', bind);
  else bind();
})();
