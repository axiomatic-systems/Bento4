from invoke import task

@task(default = True)
def build(ctx):
    ctx.run("mkdir -p cmakebuild")
    ctx.run("cd cmakebuild && cmake -DCMAKE_BUILD_TYPE=Release ..")
    ctx.run("cd cmakebuild && make")

@task(pre=[build])
def package(ctx):
    ctx.run("python3 Scripts/SdkPackager.py")
