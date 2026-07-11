var jsLib = {
  print_to_dom: function (ptr) {
    var p = document.createElement('p');
    p.textContent = Pointer_stringify(ptr);
    document.body.appendChild(p);
  },
};
mergeInto(LibraryManager.library, jsLib);
