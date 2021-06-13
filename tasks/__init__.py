"""
Tasks for Bento4
"""

import subprocess
import os
from invoke import Collection, Config
from . import build
from . import docker
from . import doc
from . import test

GIT_DIR = subprocess.check_output("git rev-parse --show-toplevel",
                                  shell=True).strip().decode("utf-8")
ROOT_DIR = GIT_DIR

# Add collections
ns = Collection()
ns.add_collection(build)
ns.add_collection(docker)
ns.add_collection(doc)
ns.add_collection(test)

# Setup the configuration
config = Config(project_location=ROOT_DIR)
config.C = {}
config.C.ROOT_DIR = ROOT_DIR
config.C.DOC_DIR = os.path.join(ROOT_DIR, "Documents")

ns.configure(config)