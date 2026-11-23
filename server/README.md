# YW Studio server (VPS)

Everything that runs on the YW Studio VPS, as code. The website stays on Hostinger (`website/`,
`scripts/deploy-site.sh`); this folder is the separate server for the app's online features.

| URL | What | Code |
|---|---|---|
| `https://api.yolo.su/health` | health of the API service | `app/yw_api.py` |
| `https://api.yolo.su/v1/shares…`, `/s/<id>` | share links for `.ywpack` files (Online Sessions P1) | `app/yw_api.py` |
| `https://relay.yolo.su/health`, `/v1/rooms…`, `wss://…/v1/rooms/<room>/ws` | WebSocket relay skeleton (Online Sessions P2/P4) | `app/yw_relay.py` |

API contract: [`docs/design/SHARE_API.md`](../docs/design/SHARE_API.md). Design background:
[`docs/design/ONLINE_SESSIONS.md`](../docs/design/ONLINE_SESSIONS.md). Crash-report move (planned, not switched):
[`docs/design/CRASH_REPORTS_VPS.md`](../docs/design/CRASH_REPORTS_VPS.md).

## Layout

```
server/
  setup.sh           idempotent installer, runs ON the VPS as root (packages, user, folders, env, code, units, ufw, Caddy)
  deploy.sh          from a workstation: copies server/ to the VPS (/opt/yw/src) and runs setup.sh
  env.example        settings and limits; setup.sh creates /etc/yw/yw.env from it (chmod 600, random salt)
  caddy/Caddyfile    reverse proxy + automatic HTTPS (Let's Encrypt), address-free access log
  app/               the two services (Python 3 + aiohttp from Ubuntu's packages; no pip, no containers)
  systemd/           yw-api, yw-relay (Restart=always, sandboxed), backup + monitor timers
  ops/               backup.sh, monitor.sh, logrotate and journald settings
  tests/smoke.py     end-to-end test (live or local)
```

## Machine

Ubuntu 26.04 LTS, 2 vCPU, 7.7 GB RAM, 96 GB disk; `ssh yw-vps` (root, key auth; alias in the maintainer's
`~/.ssh/config`). Base hardening done by hand once (2026-11-02): ufw, fail2ban (sshd), unattended-upgrades. `setup.sh`
never touches SSH settings and only *adds* firewall rules (22, 80, 443/tcp, 443/udp).

DNS (Hostinger DNS API, zone `yolo.su`): `api` and `relay` are A records → `179.66.129.177` (TTL 300). The
root, `www` and `ftp` records (website) are not touched. To recreate them:

```sh
T=$(cat ~/.config/hostinger/token)
curl -X PUT -H "Authorization: Bearer $T" -H 'Content-Type: application/json' \
  https://developers.hostinger.com/api/dns/v1/zones/yolo.su \
  -d '{"overwrite":false,"zone":[{"name":"api","type":"A","ttl":300,"records":[{"content":"179.66.129.177"}]},
                                 {"name":"relay","type":"A","ttl":300,"records":[{"content":"179.66.129.177"}]}]}'
```

(`"overwrite": false` adds/updates only the named records.) IPv6: the VPS has `2a02:4780:95:2d9b::1`; no AAAA records
yet (add them once IPv6 reachability is checked from outside).

## Deploy / redeploy

```sh
server/deploy.sh                 # copies server/, runs setup.sh; restarts the services only if code/units changed
python3 server/tests/smoke.py    # end-to-end check of the live URLs (needs aiohttp locally)
```

Local run of the services (no VPS, for app development):

```sh
cd server/app
YW_DATA_DIR=/tmp/yw YW_PUBLIC_URL=http://127.0.0.1:8081 YW_MIN_FREE_BYTES=0 python3 yw_api.py &
python3 yw_relay.py &
python3 ../tests/smoke.py http://127.0.0.1:8081 ws://127.0.0.1:8082
```

## On the server

| Path | What |
|---|---|
| `/opt/yw/app`, `/opt/yw/ops` | installed code (from `/opt/yw/src`, the last deployed copy of this folder) |
| `/etc/yw/yw.env` | settings + secrets (root, 600). Never in the repo. Edit, then `systemctl restart yw-api yw-relay` |
| `/var/lib/yw/shares/<id>/` | share metadata + file (user `yw`, 700) |
| `/var/backups/yw/` | daily archives, 14 kept (root, 700) |
| `/var/log/yw/monitor.log`, `backup.log` | one line per check / backup (logrotate weekly, 8 kept) |
| `/var/log/caddy/yw-access.log` | access log without client addresses or headers (rolled at 20 MB, 10 kept, 30 days) |

Services log to the journal (capped at 500 MB / 1 month): `journalctl -u yw-api -u yw-relay -u caddy`.

**Restarts**: systemd `Restart=always` (crashes), `yw-monitor.timer` every 5 min (hangs: two failed `/health` in a
row → restart; public HTTPS failing while the services are fine → reload Caddy; warnings for disk ≥ 85 %, memory
< 500 MB, inactive units, in the journal as `yw-monitor`). Caddy renews certificates itself.

**Backups** (`yw-backup.timer`, daily ~04:20 UTC): `/etc/yw`, `/etc/caddy`, `/var/lib/caddy` (certificates),
`/var/lib/yw` **without** share files (they are temporary, up to 60 GB, and the sender still has them). Restore:
`tar -xzf /var/backups/yw/yw-<stamp>.tar.gz -C /` then `server/deploy.sh`. The archives are on the same disk:
an off-machine copy is still open (see below).

**Limits** (all in `yw.env`): share files ≤ 2 GiB, 1–30 days (7 default), 60 GiB of shares in total, 10 GiB disk
always left free, 20 shares/hour and 8 GiB/day per client, unfinished uploads dropped after 24 h; relay 4 peers per
room, 200 rooms, 10 rooms/hour per client, 1 MiB per frame, 4 MiB/s per peer. Clients are told apart only by a salted
hash of their address kept in memory (never stored or logged). Services are bound to 127.0.0.1 and sandboxed
(`ProtectSystem=strict`, own user, memory caps 768 / 512 MB).

## Open items

- Off-machine backups (e.g. a weekly `rsync` pull to the maintainer's PC) — needs a destination decision.
- External uptime check (from outside the VPS): `curl -sf https://api.yolo.su/health` from the workstation or a
  free monitor service (needs a sign-up: the user decides).
- Crash-report move: plan in `docs/design/CRASH_REPORTS_VPS.md`.
- `PasswordAuthentication no` for SSH after the user changes the root password (not done by these scripts).
