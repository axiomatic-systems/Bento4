import os
import re
from invoke import task

def get_version():
    script_dir  = os.path.abspath(os.path.dirname(__file__))
    bento4_home = os.path.join(script_dir,'..')
    with open(bento4_home + '/Source/C++/Core/Ap4Version.h') as f:
        lines = f.readlines()
        for line in lines:
            m = re.match(r'.*AP4_VERSION_STRING *"([0-9]*)\.([0-9]*)\.([0-9]*).*"', line)
            if m:
                return m.group(1) + '.' + m.group(2) + '.' + m.group(3)
    return '0.0.0'

def get_sdk_revision():
    cmd = 'git status --porcelain -b'
    lines = os.popen(cmd).readlines()
    if not lines[0].startswith('## master'):
        print('WARNING: not on master branch')
    if len(lines) > 1:
        print('WARNING: git status not empty')
        print(''.join(lines))

    cmd = 'git tag --contains HEAD'
    tags = os.popen(cmd).readlines()
    if len(tags) != 1:
        print('ERROR: expected exactly one tag for HEAD, found', len(tags), ':', tags)
        return None
    version = tags[0].strip()
    sep = version.find('-')
    if sep < 0:
        print('ERROR: unrecognized version string format:', version)
    return version[sep+1:]

@task(default = True)
def build(ctx):
    command = "docker image build -t bento4:{version}-{revision} -t bento4:latest --build-arg BENTO4_VERSION={version}-{revision} -f Build/Docker/Dockerfile .".format(version=get_version(), revision=get_sdk_revision())
    ctx.run(command)

@task
def build_for_dev(ctx):
    command = "docker image build -t bento4-dev:latest -f Build/Docker/Dockerfile.dev ."
    ctx.run(command)

@task
def shell(ctx):
    command = "docker run --rm -it -v `pwd`:/home/bento4/project -w /home/bento4/project bento4-dev bash"
    ctx.run(command, pty=True)
