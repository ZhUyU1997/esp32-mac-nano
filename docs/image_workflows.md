# Disk Image Workflows

## Document index

- Project rules & workflows (Trae): [AGENTS.md](../AGENTS.md) — 项目规则与工作流
- HFS helper scripts:
  - [hfs-tree.sh](../scripts/hfs-tree.sh)
  - [hfs-sync.sh](../scripts/hfs-sync.sh)

## Create an image (MAME + CHD)

```bash
chdman createhd -c none -o my_macplus.chd -s 4194304
mame macplus -hard1 my_macplus.chd -flop1 ./mac6.0.5/"System Tools.img"
mame macplus -hard1 my_macplus.chd -flop1 ./mac6.0.8/"System Tools.img"
chdman extractraw -i ./my_macplus.chd -o hd.img

hcopy ./fs/Applications/* ':Applications'
```

## HFS scripts

### List image tree (scripts/hfs-tree.sh)

```bash
./scripts/hfs-tree.sh
./scripts/hfs-tree.sh ./macintosh/hd10.img
./scripts/hfs-tree.sh --visible-only ./macintosh/hd10.img
```

### Sync image contents (scripts/hfs-sync.sh)

```bash
./scripts/hfs-sync.sh SOURCE.img DEST.img
./scripts/hfs-sync.sh -d Applications -d Games SOURCE.img DEST.img
./scripts/hfs-sync.sh SOURCE.img DEST.img Applications "System Folder"
./scripts/hfs-sync.sh -n -v -d Applications SOURCE.img DEST.img
```

## Notes

- `hfs-sync.sh` is one-way sync (it does not delete extra files in DEST).
- Same-name files overwrite; if DEST already has a directory with the same name, the file is skipped with an error.
- Paths are written relative to the HFS volume root; both `:` and `/` separators are accepted (the script normalizes them).
