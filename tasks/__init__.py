"""
Tasks for Bento4
"""

from invoke import Collection
from . import build
from . import docker
from . import test

# Add collections
ns = Collection()
ns.add_collection(build)
ns.add_collection(docker)
ns.add_collection(test)
