// Folder drop, the Isabel sample, and the file picker. The config is parsed in C++.
(function () {
  const sliceBytes = 8 << 20;
  const memoryCapLabel = '2048 MB';
  const assetVersion = new URL(document.currentScript.src, window.location.href).searchParams.get('v') || '';

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
    const locked = busy || !window.traitvisRuntimeReady;
    document.getElementById('folder').disabled = locked;
    document.getElementById('sample').disabled = locked;
    document.getElementById('levelset').disabled = locked;
    const samplesHidden = document.getElementById('saved-level-sets').hidden;
    document.getElementById('sample-level-set').disabled = locked || samplesHidden;
    document.getElementById('load-sample-level-set').disabled = locked || samplesHidden;
    document.getElementById('drop').setAttribute('aria-disabled', locked ? 'true' : 'false');
  }

  function formatMegabytes(bytes) {
    return Math.round(bytes / (1024 * 1024)) + ' MB';
  }

  function residentBytes() {
    if (!window.traitvisRuntimeReady) return 0;
    return Number(Module.ccall('traitvisResidentBytes', 'string', [], []));
  }

  function selectionFits(bytes) {
    return Module.ccall('traitvisLoadFits', 'number', ['string'], [String(bytes)]) === 1;
  }

  function refreshMemory() {
    const label = document.getElementById('memory');
    if (!label) return;
    label.textContent = 'Memory: ' + formatMegabytes(residentBytes()) + ' / ' + memoryCapLabel;
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

  function describeConfig() {
    const listingPtr = Module.ccall('traitvisDescribeConfig', 'number', ['string'], ['/dataset/config.txt']);
    if (!listingPtr) throw new Error('Could not read the dataset config.');
    const listing = readCString(listingPtr);
    Module._free(listingPtr);
    if (listing.startsWith('!')) throw new Error(listing.slice(1).trim());
    const attributes = parseListing(listing);
    if (!attributes.length) throw new Error('The config has no attributes.');
    return attributes;
  }

  async function copyInSlices(file, ptr, onProgress) {
    for (let offset = 0; offset < file.size; offset += sliceBytes) {
      const end = Math.min(file.size, offset + sliceBytes);
      const bytes = new Uint8Array(await file.slice(offset, end).arrayBuffer());
      // Re-read the heap view. An earlier allocation can replace it.
      Module.HEAPU8.set(bytes, (ptr + offset) >>> 0);
      onProgress(end / file.size);
    }
  }

  async function streamInto(url, ptr, total, onProgress) {
    const response = await fetch(url);
    if (!response.ok || !response.body) throw new Error('Could not fetch ' + url + '.');
    const reader = response.body.getReader();
    let offset = 0;
    for (;;) {
      const chunk = await reader.read();
      if (chunk.done) break;
      Module.HEAPU8.set(chunk.value, (ptr + offset) >>> 0);
      offset += chunk.value.length;
      onProgress(offset / total);
    }
    if (offset !== total) throw new Error(url + ' was not the size the config describes.');
  }

  async function stageAndLoad(attributes, readBytes) {
    const total = attributes.reduce((sum, attribute) => sum + attribute.count * 4, 0);
    if (!selectionFits(total)) throw new Error('That selection is over the 2 GB limit.');

    let copied = 0;
    for (const attribute of attributes) {
      status('Copying ' + attribute.name + '…');
      const dest = Module.ccall(
        'traitvisBeginAttribute', 'number', ['string', 'number'], [attribute.file, attribute.count]) >>> 0;
      if (!dest) throw new Error('Could not allocate ' + attribute.name + '.');
      await readBytes(attribute, dest, ratio => setProgress((copied + ratio * attribute.count * 4) / total));
      copied += attribute.count * 4;
    }

    const started = Module.ccall(
      'traitvisBeginLoad', 'number', ['string', 'string'],
      ['/dataset/config.txt', attributes.map(attribute => attribute.file).join(',')]);
    if (!started) throw new Error('Could not start the load.');
    const noun = attributes.length === 1 ? 'attribute' : 'attributes';
    status('Loaded ' + attributes.length + ' ' + noun + '.');
    document.getElementById('saved-level-sets').hidden = false;
    refreshMemory();
  }

  function chooseAttributes(attributes) {
    const picker = document.getElementById('picker');
    const list = document.getElementById('picker-list');
    const note = document.getElementById('picker-note');
    const load = document.getElementById('picker-load');
    const cancel = document.getElementById('picker-cancel');
    list.replaceChildren();

    const boxes = attributes.map(attribute => {
      const label = document.createElement('label');
      const box = document.createElement('input');
      box.type = 'checkbox';
      box.checked = true;
      label.append(box, document.createTextNode(attribute.name + ' (' + formatMegabytes(attribute.count * 4) + ')'));
      list.append(label);
      return box;
    });

    function selected() {
      return attributes.filter((attribute, index) => boxes[index].checked);
    }

    function updateChoice() {
      const chosen = selected();
      const incoming = chosen.reduce((sum, attribute) => sum + attribute.count * 4, 0);
      const fits = chosen.length > 0 && selectionFits(incoming);
      load.disabled = !fits;
      if (!chosen.length) note.textContent = 'Choose at least one attribute.';
      else if (!fits) note.textContent = 'That selection is over the 2 GB limit.';
      else note.textContent = 'Selected ' + formatMegabytes(incoming) + '.';
    }

    boxes.forEach(box => box.addEventListener('change', updateChoice));
    updateChoice();
    picker.hidden = false;

    return new Promise(resolve => {
      function finish(result) {
        picker.hidden = true;
        load.onclick = null;
        cancel.onclick = null;
        resolve(result);
      }
      load.onclick = () => finish(selected());
      cancel.onclick = () => finish(null);
    });
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

  async function loadDescribed(attributes, readBytes) {
    setBusy(true);
    showError('');
    try {
      await stageAndLoad(attributes, readBytes);
    } catch (error) {
      Module.ccall('traitvisCancelPending', null, [], []);
      status('');
      showError(error && error.message ? error.message : String(error));
    } finally {
      setBusy(false);
      setProgress(0);
      refreshMemory();
    }
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
    let attributes;
    try {
      ensureDatasetDir();
      Module.FS.writeFile('/dataset/config.txt', new Uint8Array(await configFile.arrayBuffer()));
      attributes = describeConfig();
    } catch (error) {
      status('');
      showError(error && error.message ? error.message : String(error));
      setBusy(false);
      return;
    }

    // The folder and sample buttons stay disabled while the visitor is choosing.
    const chosen = await chooseAttributes(attributes);
    if (!chosen) {
      status('Drop a folder, or load the sample.');
      setBusy(false);
      refreshMemory();
      return;
    }

    await loadDescribed(chosen, async (attribute, dest, onProgress) => {
      const file = files.find(item => item.name === attribute.file);
      if (!file) throw new Error('Missing attribute file ' + attribute.file + '.');
      if (file.size !== attribute.count * 4) {
        throw new Error(attribute.file + ' is not the size the config describes.');
      }
      await copyInSlices(file, dest, onProgress);
    });
  }

  async function loadSample() {
    setBusy(true);
    showError('');
    status('Reading sample…');
    let attributes;
    try {
      const response = await fetch('isabel/config.txt');
      if (!response.ok) throw new Error('The Isabel sample config is not next to this page.');
      ensureDatasetDir();
      Module.FS.writeFile('/dataset/config.txt', new Uint8Array(await response.arrayBuffer()));
      attributes = describeConfig();
    } catch (error) {
      status('');
      showError(error && error.message ? error.message : String(error));
      setBusy(false);
      return;
    }

    await loadDescribed(attributes, async (attribute, dest, onProgress) => {
      await streamInto('isabel/' + attribute.file, dest, attribute.count * 4, onProgress);
    });
  }

  function submitLevelSet(ptr, byteCount, label) {
    const message = Module.ccall(
      'traitvisOpenLevelSet', 'string', ['string', 'string'],
      [String(ptr >>> 0), String(byteCount)]);
    if (message) showError(message);
    status(message ? '' : 'Opening ' + label + '…');
  }

  async function openLevelSet(file) {
    if (!file || file.size === 0) {
      showError('That file is not a TraitVis level set.');
      return;
    }
    setBusy(true);
    showError('');
    status('Reading ' + file.name + '…');
    const ptr = Module._malloc(file.size);
    if (!ptr) {
      showError('Could not allocate memory for that level set.');
      setBusy(false);
      return;
    }
    try {
      await copyInSlices(file, ptr >>> 0, setProgress);
      submitLevelSet(ptr, file.size, file.name);
    } catch (error) {
      showError(error && error.message ? error.message : String(error));
      status('');
    } finally {
      Module._free(ptr);
      setProgress(0);
      setBusy(false);
    }
  }

  async function openSavedLevelSet(url, label) {
    setBusy(true);
    showError('');
    status('Reading ' + label + '…');
    let ptr = 0;
    try {
      const response = await fetch(url);
      if (!response.ok || !response.body) throw new Error('Could not fetch ' + label + '.');
      const total = Number(response.headers.get('Content-Length'));
      if (!total) throw new Error(label + ' was not the expected size.');
      ptr = Module._malloc(total);
      if (!ptr) throw new Error('Could not allocate memory for that level set.');
      const reader = response.body.getReader();
      let offset = 0;
      for (;;) {
        const chunk = await reader.read();
        if (chunk.done) break;
        Module.HEAPU8.set(chunk.value, (ptr + offset) >>> 0);
        offset += chunk.value.length;
        setProgress(offset / total);
      }
      if (offset !== total) throw new Error(label + ' was not the expected size.');
      submitLevelSet(ptr, total, label);
    } catch (error) {
      showError(error && error.message ? error.message : String(error));
      status('');
    } finally {
      if (ptr) Module._free(ptr);
      setProgress(0);
      setBusy(false);
    }
  }

  window.traitvisLoadFolder = loadFolder;

  function bind() {
    const folder = document.getElementById('folder');
    const drop = document.getElementById('drop');
    const sample = document.getElementById('sample');
    folder.addEventListener('change', () => {
      if (folder.files && folder.files.length) loadFolder(folder.files);
      folder.value = '';
    });
    sample.addEventListener('click', () => loadSample());
    const levelSetButton = document.getElementById('levelset');
    const levelSetFile = document.getElementById('levelset-file');
    levelSetButton.addEventListener('click', () => levelSetFile.click());
    levelSetFile.addEventListener('change', () => {
      const file = levelSetFile.files && levelSetFile.files[0];
      levelSetFile.value = '';
      if (file) openLevelSet(file);
    });
    document.getElementById('load-sample-level-set').addEventListener('click', () => {
      const select = document.getElementById('sample-level-set');
      const option = select.selectedOptions[0];
      if (!option || select.disabled) return;
      openSavedLevelSet(option.value, option.textContent);
    });
    drop.addEventListener('dragover', event => {
      event.preventDefault();
      if (window.traitvisRuntimeReady && !folder.disabled) drop.classList.add('drag');
    });
    drop.addEventListener('dragleave', () => drop.classList.remove('drag'));
    drop.addEventListener('drop', async event => {
      event.preventDefault();
      drop.classList.remove('drag');
      if (!window.traitvisRuntimeReady || folder.disabled) return;
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
    status('Drop a folder, or load the sample.');
    refreshMemory();
    window.setInterval(refreshMemory, 500);
  };

  const canvas = document.getElementById('canvas');
  const fallback = document.getElementById('webgl-fallback');
  // The shell minifier drops braces in an inline script, so the runtime object lives here.
  window.Module = {
    canvas: canvas,
    locateFile: function (file, prefix) {
      const name = assetVersion ? file + '?v=' + assetVersion : file;
      return prefix ? prefix + name : name;
    },
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
