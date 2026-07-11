emcc helper.cpp -I ../C++/Core -I ../C++/Metadata -I ../C++/Codecs \
  -lap4 -L../../builds/ \
  -s EXPORTED_FUNCTIONS="['_dump_info']" \
  -s TOTAL_MEMORY=268435456 \
  --js-library jslib.js \
  -std=c++14 -o helper.js
