document.addEventListener('DOMContentLoaded', function () {
  var filepicker = document.getElementById('filepicker');
  var reader = new FileReader;
  var dump_info = Module.cwrap('dump_info', 'number', ['number', 'number']);
  filepicker.addEventListener('change', function () {
    reader.readAsArrayBuffer(filepicker.files[0]);
  });
  reader.addEventListener('loadend', function (e) {
    var view = new Uint8Array(reader.result);
    var length = view.length * view.BYTES_PER_ELEMENT;
    var inPtr = Module._malloc(length);
    Module.HEAPU8.set(view, inPtr);
    var ret = dump_info(inPtr, length);
    Module._free(inPtr);
  });
});
