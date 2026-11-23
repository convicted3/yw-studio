#!/usr/bin/env bash
# YW Studio server setup — idempotent; run as root ON THE VPS from a copy of the repo's server/ folder.
# Normally started by server/deploy.sh from a workstation (copies server/ to /opt/yw/src and runs this).
# Safe to run again at any time: it installs/updates packages, code, Caddy config, units and timers, never touches
# SSH settings, keeps /etc/yw/yw.env (secrets) and all data.
set -euo pipefail
[ "$(id -u)" = 0 ] || { echo "run as root"; exit 1; }
SRC=$(cd "$(dirname "$0")" && pwd)
export DEBIAN_FRONTEND=noninteractive

echo "== packages"
need=()
for p in caddy python3-aiohttp curl logrotate ufw; do dpkg -s "$p" >/dev/null 2>&1 || need+=("$p"); done
if [ ${#need[@]} -gt 0 ]; then apt-get update -q; apt-get install -y -q --no-install-recommends "${need[@]}"; fi

echo "== user and folders"
id yw >/dev/null 2>&1 || useradd --system --home-dir /var/lib/yw --shell /usr/sbin/nologin yw
install -d -m 755 /opt/yw /opt/yw/app /opt/yw/ops
install -d -m 700 -o yw -g yw /var/lib/yw /var/lib/yw/shares
install -d -m 700 /etc/yw /var/backups/yw
install -d -m 750 -g adm /var/log/yw
install -d -m 755 -o caddy -g caddy /var/log/caddy

echo "== secrets (/etc/yw/yw.env, kept if present)"
if [ ! -f /etc/yw/yw.env ]; then
  umask 077
  sed "s/^YW_SALT=.*/YW_SALT=$(head -c 32 /dev/urandom | od -An -tx1 | tr -d ' \n')/" "$SRC/env.example" > /etc/yw/yw.env
fi
chmod 600 /etc/yw/yw.env; chown root:root /etc/yw/yw.env
# new keys added to env.example later are appended with their defaults (existing values are never changed)
while IFS= read -r line; do
  k=${line%%=*}; case "$line" in ''|\#*) continue;; esac
  grep -q "^$k=" /etc/yw/yw.env || echo "$line" >> /etc/yw/yw.env
done < "$SRC/env.example"

echo "== code"
changed=0
for f in "$SRC"/app/*.py; do
  cmp -s "$f" "/opt/yw/app/$(basename "$f")" || { install -m 644 "$f" /opt/yw/app/; changed=1; }
done
install -m 755 "$SRC"/ops/backup.sh "$SRC"/ops/monitor.sh /opt/yw/ops/
install -m 644 "$SRC"/ops/logrotate-yw /etc/logrotate.d/yw
install -d /etc/systemd/journald.conf.d
if ! cmp -s "$SRC/ops/journald-yw.conf" /etc/systemd/journald.conf.d/yw.conf; then
  install -m 644 "$SRC/ops/journald-yw.conf" /etc/systemd/journald.conf.d/yw.conf
  systemctl restart systemd-journald
fi
python3 -m py_compile /opt/yw/app/*.py && rm -rf /opt/yw/app/__pycache__

echo "== systemd units"
units_changed=0
for f in "$SRC"/systemd/*; do
  cmp -s "$f" "/etc/systemd/system/$(basename "$f")" || { install -m 644 "$f" /etc/systemd/system/; units_changed=1; }
done
systemctl daemon-reload
systemctl enable -q yw-api yw-relay yw-backup.timer yw-monitor.timer caddy
if [ $changed = 1 ] || [ $units_changed = 1 ]; then systemctl restart yw-api yw-relay; else systemctl start yw-api yw-relay; fi
systemctl start yw-backup.timer yw-monitor.timer

echo "== firewall (22 stays open; adds 80/443 for Caddy)"
ufw allow 22/tcp >/dev/null
ufw allow 80/tcp >/dev/null
ufw allow 443/tcp >/dev/null
ufw allow 443/udp >/dev/null   # HTTP/3
ufw status | grep -q 'Status: active' || { echo "ufw inactive: enable it by hand after checking (ufw enable)"; }

echo "== caddy"
caddy validate --config "$SRC/caddy/Caddyfile" --adapter caddyfile >/dev/null 2>&1 || { caddy validate --config "$SRC/caddy/Caddyfile" --adapter caddyfile; exit 1; }
if ! cmp -s "$SRC/caddy/Caddyfile" /etc/caddy/Caddyfile; then
  install -m 644 "$SRC/caddy/Caddyfile" /etc/caddy/Caddyfile
  systemctl reload-or-restart caddy
fi
systemctl is-active -q caddy || systemctl start caddy

echo "== local health"
for i in 1 2 3 4 5 6 7 8 9 10; do
  if curl -sf -m 5 http://127.0.0.1:8081/health >/dev/null && curl -sf -m 5 http://127.0.0.1:8082/health >/dev/null; then break; fi
  sleep 1
done
curl -sf -m 5 http://127.0.0.1:8081/health; echo
curl -sf -m 5 http://127.0.0.1:8082/health; echo
echo "setup done"
