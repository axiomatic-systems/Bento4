from invoke import task, Collection

@task(help = {
    'coverage': "Enable code coverage analysis"
    }, default = True)
def run(ctx, coverage=False):
    command = "python3 -m pytest Test/Pytest"
    if coverage:
        command = "coverage3 run --source=Source/Python/utils -m pytest Test/Pytest"
    ctx.run(command, env={'PYTHONPATH': 'Source/Python/utils', 'BENTO4_HOME': os.path.abspath(os.path.curdir)})
    if coverage:
        ctx.run("coverage3 html -d coverage_html")
