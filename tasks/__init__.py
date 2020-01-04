"""
Tasks for Bento4
"""

from invoke import task

test_commands = [
    'Source/Python/utils/mp4-dash.py -o test_tmp/1 ./Test/Data/test-002.mp4',
    'Source/Python/utils/mp4-dash.py -o test_tmp/2 -d ./Test/Data/test-002.mp4',
    'Source/Python/utils/mp4-dash.py -o test_tmp/3 -v ./Test/Data/test-002.mp4',
    'Source/Python/utils/mp4-dash.py -o test_tmp/4 -d -v ./Test/Data/test-002.mp4',
    'Source/Python/utils/mp4-dash.py -o test_tmp/5 --max-playout-rate lowest:1000 ./Test/Data/test-002.mp4'
]
@task(help = {
    'coverage': "Enable code coverage analysis"
})
def test(ctx, coverage=False):
    ctx.run("rm -rf test_tmp")
    ctx.run("mkdir test_tmp")
    for command in test_commands:
        if coverage:
            command = "coverage run " + command
        else:
            command = "python " + command
        ctx.run(command)
    if coverage:
        ctx.run("coverage html -d coverage_html")
