#!/usr/bin/env bash
# Daily backup of the YW Studio service data (run by yw-backup.timer as root).
# Keeps YW_BACKUP_KEEP (default 14) daily archives in /var/backups/yw (chmod 700, archives 600).
# Share blobs (/var/lib/yw/shares/*/blob) are NOT backed up: they are temporary (they expire in days), can be up to
# 60 GB in total and the host re-uploads a link that was lost. Their metadata is backed up.
set -euo pipefail
DEST=/var/backups/yw
KEEP=${YW_BACKUP_KEEP:-14}
install -d -m 700 "$DEST"
STAMP=$(date -u +%Y%m%d-%H%M)
OUT="$DEST/yw-$STAMP.tar.gz"
umask 077
paths=()
for p in /etc/yw /etc/caddy /var/lib/caddy /var/lib/yw; do [ -e "$p" ] && paths+=("$p"); done
tar --create --gzip --file "$OUT.part" --warning=no-file-changed --exclude='/var/lib/yw/shares/*/blob' \
    --exclude='*.tmp' "${paths[@]}" 2>/dev/null || [ $? -eq 1 ]
mv "$OUT.part" "$OUT"
# rotation: newest $KEEP archives stay
ls -1t "$DEST"/yw-*.tar.gz 2>/dev/null | tail -n +"$((KEEP + 1))" | xargs -r rm -f
echo "$(date -u +%FT%TZ) backup $OUT $(stat -c %s "$OUT") bytes, $(ls -1 "$DEST"/yw-*.tar.gz | wc -l) kept" >> /var/log/yw/backup.log
