import sys
import os
from invoke import task

@task(default = True, help = { 'clean': 'Clean before building'})
def build(ctx, clean = False):
    if clean:
        ctx.run("rm -rf cmakebuild")
    try:
        os.makedirs("cmakebuild")
    except:
        pass
    with ctx.cd("cmakebuild"):
        generator = ''
        if sys.platform == 'darwin':
            generator = '-GXcode'
        command = "cmake {} -DCMAKE_BUILD_TYPE=Release ..".format(generator)
        ctx.run(command)
        ctx.run("cmake --build . --config Release")

@task
def sdk(ctx):
    ctx.run("python3 Scripts/SdkPackager.py")
