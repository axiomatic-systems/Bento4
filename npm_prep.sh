cd builds
echo "#!/usr/bin/env node" | cat - mp4dump.js > /tmp/out && \
  mv /tmp/out mp4dump.js
chmod +x mp4dump.js && echo "done"
