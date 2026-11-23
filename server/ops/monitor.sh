#!/usr/bin/env bash
# YW Studio health + resource check (yw-monitor.timer, every 5 minutes, as root).
# - restarts a service whose /health fails twice in a row (systemd's Restart=always covers crashes; this covers hangs)
# - checks the public HTTPS path through Caddy (certificate, DNS, proxy) and reloads Caddy if only that fails
# - logs one line per run to /var/log/yw/monitor.log; warnings also go to the journal (journalctl -t yw-monitor)
set -uo pipefail
LOG=/var/log/yw/monitor.log
STATE=/run/yw-monitor; mkdir -p "$STATE"
warn() { logger -t yw-monitor -p user.warning "$*"; echo "$(date -u +%FT%TZ) WARN $*" >> "$LOG"; }

check() { # name url unit
  if curl -sf -m 10 -o /dev/null "$2"; then rm -f "$STATE/$1.fail"; echo ok; return; fi
  if [ -e "$STATE/$1.fail" ]; then warn "$1 health failed twice: restarting $3"; systemctl restart "$3"; rm -f "$STATE/$1.fail"
  else touch "$STATE/$1.fail"; fi
  echo FAIL
}
api=$(check api http://127.0.0.1:8081/health yw-api)
relay=$(check relay http://127.0.0.1:8082/health yw-relay)
pub=ok
for h in api relay; do
  if ! curl -sf -m 15 -o /dev/null "https://$h.yolo.su/health"; then pub=FAIL; fi
done
if [ "$pub" = FAIL ] && [ "$api" = ok ] && [ "$relay" = ok ]; then
  if [ -e "$STATE/public.fail" ]; then warn "public HTTPS failed twice: reloading caddy"; systemctl reload-or-restart caddy; rm -f "$STATE/public.fail"
  else touch "$STATE/public.fail"; fi
else rm -f "$STATE/public.fail"; fi

load=$(cut -d' ' -f1-3 /proc/loadavg)
mem=$(free -m | awk '/^Mem:/ {printf "%d/%dMB", $3, $2}')
disk=$(df -P / | awk 'NR==2 {print $5}')
diskpct=${disk%\%}
shares=$(du -sm /var/lib/yw/shares 2>/dev/null | cut -f1)
echo "$(date -u +%FT%TZ) api=$api relay=$relay https=$pub load=[$load] mem=$mem disk=$disk shares=${shares:-0}MB" >> "$LOG"
[ "$diskpct" -ge 85 ] && warn "disk at $disk"
avail=$(free -m | awk '/^Mem:/ {print $7}'); [ "$avail" -lt 500 ] && warn "low memory: ${avail}MB available"
for u in yw-api yw-relay caddy; do systemctl is-active -q "$u" || warn "$u is not active"; done
exit 0
