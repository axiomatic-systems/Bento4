"""
Tasks for Bento4
"""

from invoke import Collection
from . import sdk
from . import docker
from . import test

# Add collections
ns = Collection()
ns.add_collection(sdk)
ns.add_collection(test)
ns.add_collection(docker)
