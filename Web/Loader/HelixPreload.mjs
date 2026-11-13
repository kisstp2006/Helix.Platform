// The loader of HelixPreload.js as an ES module, for pages that start the module themselves (a .NET web app's
// dotnet.js): helixPreload(options) answers a preRun function that fetches the files into the module's file system
// before main, and helixArguments() the command line of the page's address (?args=). The options are those of
// Module.helixPreload in HelixPreload.js (url, mount, files, lists, companions; ?data=<URL> replaces url).

export function helixArguments() {
  const params = new URLSearchParams(typeof location !== 'undefined' ? location.search : '');
  return (params.get('args') || '').split(' ').filter((a) => a.length > 0);
}

export function helixPreload(options) {
  const params = new URLSearchParams(typeof location !== 'undefined' ? location.search : '');
  const base = (params.get('data') || options.url || '.').replace(/\/+$/, '');
  const mount = (options.mount || '/').replace(/\/+$/, '');
  return (module) => {
    const FS = module.FS;
    module.addRunDependency('helix-preload');
    if (mount) {
      FS.mkdirTree(mount);
    }

    const fetchFile = async (path, optional) => {
      const response = await fetch(base + '/' + path);
      if (!response.ok) {
        if (optional) {
          return null;
        }
        throw new Error(path + ': HTTP ' + response.status);
      }
      const buffer = await response.arrayBuffer();
      const full = mount + '/' + path;
      FS.mkdirTree(full.substring(0, full.lastIndexOf('/')) || '/');
      FS.writeFile(full, new Uint8Array(buffer));
      return buffer;
    };

    const listed = (buffer) => new TextDecoder().decode(buffer).split('\n')
      .map((line) => line.trim())
      .filter((line) => line.length > 0 && line[0] !== '#')
      .map((line) => (/^[0-9a-fA-F]{32} /.test(line) ? line.substring(33) : line));

    const companions = options.companions || {};
    const singles = (options.files || []).map((f) => fetchFile(f, true));
    const lists = (options.lists || []).map(async (f) => {
      const buffer = await fetchFile(f, true);
      if (!buffer) {
        return [];
      }
      const loads = [];
      for (const path of listed(buffer)) {
        loads.push(fetchFile(path, false));
        for (const ext of Object.keys(companions)) {
          if (path.toLowerCase().endsWith(ext)) {
            loads.push(fetchFile(path.substring(0, path.length - ext.length) + companions[ext], true));
          }
        }
      }
      return Promise.all(loads);
    });
    Promise.all(singles.concat(lists)).then(() => module.removeRunDependency('helix-preload'), (error) => {
      console.error('Helix: ' + base + ' could not be loaded: ' + error);
      module.removeRunDependency('helix-preload');
    });
  };
}
