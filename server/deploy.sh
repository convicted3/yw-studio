#!/usr/bin/env bash
# Deploy the repo's server/ folder to the YW Studio VPS and run setup.sh there (idempotent).
#   server/deploy.sh [ssh-host]      default host: yw-vps (alias in ~/.ssh/config)
# Then: python3 server/tests/smoke.py   (end-to-end check against the live URLs)
set -euo pipefail
HOST=${1:-yw-vps}
cd "$(dirname "$0")"
tar --create --gzip --exclude='__pycache__' --exclude='*.pyc' --exclude='*.env' . \
  | ssh "$HOST" 'set -e; rm -rf /opt/yw/src.new; mkdir -p /opt/yw/src.new; tar -xzf - -C /opt/yw/src.new;
                 rm -rf /opt/yw/src; mv /opt/yw/src.new /opt/yw/src; bash /opt/yw/src/setup.sh'
