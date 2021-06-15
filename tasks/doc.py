"""Tasks to build the Bento4 documentation"""

import os
from invoke import task, Collection

@task(default=True)
def mkdocs_build(ctx):
    '''Generate the Bento4 documentation using MkDocs'''

    # Build the HTML docs
    mkdocs_dir = os.path.join(ctx.C.DOC_DIR, 'MkDocs')
    with ctx.cd(mkdocs_dir):
        ctx.run("mkdocs build")

    print("HTML docs built, you can now open {}/site/index.html".format(mkdocs_dir))

@task
def mkdocs_serve(ctx):
    '''Start a local mkdocs server'''
    mkdocs_dir = os.path.join(ctx.C.DOC_DIR, 'MkDocs')
    with ctx.cd(mkdocs_dir):
        ctx.run("mkdocs serve")

mkdocs = Collection('mkdocs')
mkdocs.add_task(mkdocs_build, 'build')
mkdocs.add_task(mkdocs_serve, 'serve')

# Export a namespace
ns = Collection(mkdocs)
