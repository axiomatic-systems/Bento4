This target builds Bento4 binaries instrumented for the AFL fuzzer. See
https://code.google.com/p/american-fuzzy-lop/

AFL needs to be installed for this target to build.

The fuzzer converage is still very small as we need to build a set of sample
files to seed it properly. However a first pass at fuzzing mp4info showed no
issues with the code.

$ afl-fuzz -i input -o output \
~/Development/Bento4/Build/Targets/x86-afl-linux/Debug/mp4info \
~/Development/Test/Bento4Afl/input/init.mp4

Gave us:

afl-fuzz 0.21b
--------------

Queue cycle: 1877

    Overall run time : 1 day, 0 hrs, 51 min, 22.20 sec
      Problems found : 0 crashes (0 unique), 0 hangs (0 unique)
       Last new path : none seen yet
   Last unique crash : none seen yet

In-depth stats:

     Execution paths : 0+0/1 done (0.00%), 0 variable
       Current stage : havoc, 2085/5000 done (41.70%)            )
    Execution cycles : 12431560 (138.93 per second)
      Bitmap density : 3734 tuples seen (11.40%)
  Fuzzing efficiency : path = 0.00, crash = 0.00, hang = 0.00 ppm

     Bit flip yields : 0/105344, 0/105343, 0/105341
    Byte flip yields : 0/13168, 0/13167, 0/13165
  Arithmetics yields : 0/632064, 0/481541, 0/310481
    Known int yields : 0/109114, 0/479430, 0/681306
  Havoc stage yields : 0/9380000 (0 latent paths)

^C
+++ Testing aborted by user +++
[+] We're done here. Have a nice day!

