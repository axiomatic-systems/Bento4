DOC_ROOT=Documents/MkDocs/src/documentation
for tool in mp4info mp4dump mp4edit mp4extract mp4encrypt mp4decrypt mp4dcfpackager mp4compact mp4fragment mp4split mp4tag mp4mux mp42aac mp42avc mp42hevc mp42hls mp42ts mp4dash mp4dashclone mp4hls
do
  echo "#" $tool > $DOC_ROOT/$tool.md
  echo '```' >> $DOC_ROOT/$tool.md
  $tool >> $DOC_ROOT/$tool.md 2>&1
  echo '```' >> $DOC_ROOT/$tool.md
done
