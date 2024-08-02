import sys
import os
import shutil
from invoke import task

def get_target_name():
    platform_to_target_map = {
        'linux':  'x86_64-unknown-linux',
        'win32':  'x86_64-microsoft-win32',
        'darwin': 'universal-apple-macosx'
    }
    return platform_to_target_map[sys.platform]

@task(default = True, help = { 'clean': 'Clean before building'})
def build(ctx, clean = False):
    target = get_target_name()
    target_dir = "cmakebuild/{}".format(target)
    if clean:
        shutil.rmtree(target_dir, ignore_errors=True)
    try:
        os.makedirs(target_dir)
    except FileExistsError:
        pass
    with ctx.cd(target_dir):
        generator = ''
        if sys.platform == 'darwin':
            generator = '-GXcode'
        command = "cmake {} -DCMAKE_BUILD_TYPE=Release ../..".format(generator)
        ctx.run(command)
        ctx.run("cmake --build . --config Release")

@task(post=[build])
def rebuild(ctx, clean = False):
    target = get_target_name()
    target_dir = "cmakebuild/{}".format(target)
    shutil.rmtree(target_dir, ignore_errors=True)

@task(rebuild)
def sdk(ctx):
    if sys.platform == "win32":
        python = "python"
    else:
        python = "python3"
    ctx.run(f"{python} Scripts/SdkPackager.py")
