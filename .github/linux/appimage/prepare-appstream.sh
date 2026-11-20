#!/usr/bin/env bash
set -euo pipefail

metainfo_dir="$1/usr/share/metainfo"
app_id="omodsim$2"
mv "$metainfo_dir/$app_id.metainfo.xml" "$metainfo_dir/$app_id.appdata.xml"
appstreamcli validate --nonet "$metainfo_dir/$app_id.appdata.xml"
