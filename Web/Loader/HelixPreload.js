// The loader of a Helix Web application (linked as --pre-js by helix_add_web_app). Before main it fetches files into
// the file system as Module.helixPreload asks, and takes the command line from the page's address:
//   Module.helixPreload = {
//     url: 'Data',                  // the folder on the server (?data=<URL> replaces it)
//     mount: '/Data',               // where its files go in the file system
//     files: ['settings.asset'],    // files fetched when they are there
//     lists: ['assets.index'],      // list files: per line a path, or a 32 digit guid, a space and a path; every
//                                   // listed file is fetched
//     companions: {'.dll': '.pdb'}  // files next to listed ones, fetched when they are there
//   };
//   ?args=<arguments separated by spaces> adds to the command line.
var Module = typeof Module !== 'undefined' ? Module : {};
(function () {
  var params = new URLSearchParams(typeof location !== 'undefined' ? location.search : '');
  var extra = (params.get('args') || '').split(' ').filter(function (a) { return a.length > 0; });
  Module.arguments = (Module.arguments || []).concat(extra);
  var preload = Module.helixPreload;
  if (!preload) {
    return;
  }
  var base = (params.get('data') || preload.url || '.').replace(/\/+$/, '');
  var mount = (preload.mount || '/').replace(/\/+$/, '');

  Module.preRun = [].concat(Module.preRun || [], [function () {
    addRunDependency('helix-preload');
    if (mount) {
      FS.mkdirTree(mount);
    }

    function fetchFile(path, optional) {
      return fetch(base + '/' + path).then(function (response) {
        if (!response.ok) {
          if (optional) {
            return null;
          }
          throw new Error(path + ': HTTP ' + response.status);
        }
        return response.arrayBuffer();
      }).then(function (buffer) {
        if (buffer) {
          var full = mount + '/' + path;
          FS.mkdirTree(full.substring(0, full.lastIndexOf('/')) || '/');
          FS.writeFile(full, new Uint8Array(buffer));
        }
        return buffer;
      });
    }

    function listed(buffer) {
      var paths = [];
      new TextDecoder().decode(buffer).split('\n').forEach(function (line) {
        line = line.trim();
        if (line.length === 0 || line[0] === '#') {
          return;
        }
        paths.push(/^[0-9a-fA-F]{32} /.test(line) ? line.substring(33) : line);
      });
      return paths;
    }

    var companions = preload.companions || {};
    var singles = (preload.files || []).map(function (f) { return fetchFile(f, true); });
    var lists = (preload.lists || []).map(function (f) {
      return fetchFile(f, true).then(function (buffer) {
        if (!buffer) {
          return [];
        }
        var loads = [];
        listed(buffer).forEach(function (path) {
          loads.push(fetchFile(path, false));
          Object.keys(companions).forEach(function (ext) {
            if (path.toLowerCase().endsWith(ext)) {
              loads.push(fetchFile(path.substring(0, path.length - ext.length) + companions[ext], true));
            }
          });
        });
        return Promise.all(loads);
      });
    });
    Promise.all(singles.concat(lists)).then(function () {
      removeRunDependency('helix-preload');
    }, function (error) {
      (Module.printErr || console.error)('Helix: ' + base + ' could not be loaded: ' + error);
      removeRunDependency('helix-preload');
    });
  }]);
})();
