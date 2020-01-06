import os
from invoke import task

def get_sdk_revision():
    cmd = 'git status --porcelain -b'
    lines = os.popen(cmd).readlines()
    branch = ''
    if not lines[0].startswith('## master'):
        print('WARNING: not on master branch')
        branch = '+' + lines[0][3:].strip()
    if len(lines) > 1:
        print('ERROR: git status not empty')
        print(''.join(lines))
        return None

    cmd = 'git tag --contains HEAD'
    tags = os.popen(cmd).readlines()
    if len(tags) != 1:
        print('ERROR: expected exactly one tag for HEAD, found', len(tags), ':', tags)
        return None
    version = tags[0].strip()
    sep = version.find('-')
    if sep < 0:
        print('ERROR: unrecognized version string format:', version)
    return version[sep+1:] + branch

@task(default = True)
def build(ctx):
    command = "echo docker image build -t bento4:{} -t bento4:latest -f Build/Docker/Dockerfile .".format(get_sdk_revision())
    ctx.run(command)
