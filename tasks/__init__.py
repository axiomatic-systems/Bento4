"""
Tasks for Bento4
"""

import os
from invoke import task

@task(help = {
    'coverage': "Enable code coverage analysis"
})
def test(ctx, coverage=False):
    command = "python3 -m pytest Test/Pytest"
    if coverage:
        command = "coverage3 run --source=Source/Python/utils -m pytest Test/Pytest"
    ctx.run(command, env={'PYTHONPATH': 'Source/Python/utils', 'BENTO4_HOME': os.path.abspath(os.path.curdir)})
    if coverage:
        ctx.run("coverage3 html -d coverage_html")
