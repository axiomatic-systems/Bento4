import sys
import os
from invoke import task

@task(default = True)
def build(ctx):
    try:
        os.makedirs("cmakebuild")
    except:
        pass
    with ctx.cd("cmakebuild"):
        ctx.run("rm CMakeCache.txt")
        generator = ''
        if sys.platform == 'darwin':
            generator = '-GXcode'
        command = "cmake {} -DCMAKE_BUILD_TYPE=Release ..".format(generator)
        ctx.run(command)
        ctx.run("cmake --build . --config Release")
