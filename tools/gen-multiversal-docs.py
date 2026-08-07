#!/usr/bin/env python3
"""Generate Markdown API docs from Multiversal Interfaces defs/*.yaml.

Usage: python3 tools/gen-multiversal-docs.py [defs_dir] [out_dir]
"""
import os
import sys
import yaml

DEFS_DIR = sys.argv[1] if len(sys.argv) > 1 else "ref/Retro68/multiversal/defs"
OUT_DIR = sys.argv[2] if len(sys.argv) > 2 else "docs/multiversal-interfaces"

def fmt_type(t):
    return t or "void"

def fmt_arg(a):
    s = a.get("type", "?")
    if a.get("register"):
        s += f" (reg {a['register']})"
    if a.get("comment"):
        c = a["comment"].splitlines()[0].strip()
        s += f" /* {c} */"
    return s

def fmt_function(f):
    ret = fmt_type(f.get("return"))
    args = ", ".join(f"{fmt_type(a.get('type'))} {a.get('name','?')}" for a in f.get("args", []))
    if f.get("varargs"):
        args += ", ..."
    return f"{ret} {f['name']}({args})"


def fmt_trap(trap):
    if isinstance(trap, int):
        return f"`0x{trap:X}`"
    if trap:
        return f"`{trap}`"
    return "— (executor 实现，无 trap)"

def fmt_enum(e):
    if e.get("name"):
        return f"enum {e['name']}"
    return "enum (anonymous)"

def clean_comment(c):
    if not c:
        return ""
    lines = [l.strip() for l in c.splitlines() if l.strip()]
    return " ".join(lines)

def parse_file(path):
    with open(path, encoding="utf-8") as f:
        try:
            data = yaml.safe_load(f)
        except yaml.YAMLError as e:
            print(f"  ! YAML error in {path}: {e}")
            return None
    if data is None:
        return None
    out = {"functions": [], "typedefs": [], "enums": [], "structs": [],
           "unions": [], "funptrs": [], "commons": [], "lowmems": [], "dispatchers": []}
    for item in data:
        api = item.get("api")
        body = {k: v for k, v in item.items() if k != "api"}
        if "function" in body:
            f = body["function"]
            f["_api"] = api
            out["functions"].append(f)
        elif "typedef" in body:
            out["typedefs"].append(body["typedef"])
        elif "enum" in body:
            out["enums"].append(body["enum"])
        elif "struct" in body:
            s = body["struct"]; s["_api"] = api
            out["structs"].append(s)
        elif "union" in body:
            out["unions"].append(body["union"])
        elif "funptr" in body:
            out["funptrs"].append(body["funptr"])
        elif "common" in body:
            out["commons"].append(body["common"])
        elif "lowmem" in body:
            out["lowmems"].append(body["lowmem"])
        elif "dispatcher" in body:
            out["dispatchers"].append(body["dispatcher"])
    return out

def type_section(title, items, fmt):
    if not items:
        return ""
    lines = [f"## {title}", ""]
    for it in items:
        name = it.get("name", "?")
        comment = clean_comment(it.get("comment"))
        extra = ""
        if title == "Typedefs":
            extra = f" = {it.get('type', '?')}"
        elif title in ("Structs", "Unions"):
            members = ", ".join(f"{m.get('name','?')}: {m.get('type','?')}"
                                for m in it.get("members", []))
            extra = f" {{ {members} }}"
        elif title == "Function Pointers":
            args = ", ".join(f"{a.get('name','?')}: {a.get('type','?')}"
                             for a in it.get("args", []))
            extra = f" ({args}) -> {it.get('return', 'void')}"
        elif title == "Common Blocks":
            members = ", ".join(f"{m.get('name','?')}: {m.get('type','?')}"
                                for m in it.get("members", []))
            extra = f" {{ {members} }}"
        elif title == "Dispatchers":
            extra = f" — {it.get('comment', it.get('description',''))}".strip()
            if it.get("nentries"):
                extra += f" ({it['nentries']} entries)"
        elif title == "Low Memory Globals":
            extra = f" @ 0x{it.get('address',0):X} ({it.get('type','?')})"
        if comment:
            extra += f" — {comment}"
        lines.append(f"- **{name}**{extra}")
    lines.append("")
    return "\n".join(lines)

def render_domain(name, parsed):
    title = name + " Interfaces"
    out = [f"# {title}", ""]
    # find a lead comment from the first item with one
    lead = next((clean_comment(it.get("comment")) for it in
                 (parsed["functions"] + parsed["typedefs"] + parsed["enums"]
                  + parsed["structs"] + parsed["lowmems"])
                 if it.get("comment") and len(clean_comment(it.get("comment"))) <= 200), None)
    if lead:
        out += [lead, ""]
    out += [f"Source: `multiversal/defs/{name}.yaml`", ""]
    out += [f"- Functions: **{len(parsed['functions'])}**",
            f"- Typedefs: **{len(parsed['typedefs'])}**",
            f"- Structs: **{len(parsed['structs'])}**, Unions: **{len(parsed['unions'])}**",
            f"- Enums: **{len(parsed['enums'])}**",
            f"- Function pointers: **{len(parsed['funptrs'])}**",
            f"- Common blocks: **{len(parsed['commons'])}**",
            f"- Dispatchers: **{len(parsed['dispatchers'])}**",
            f"- Low-memory globals: **{len(parsed['lowmems'])}**", ""]

    if parsed["functions"]:
        out += ["## Functions", ""]
        # sort classic first, then carbon
        def sortkey(f):
            return (0 if f.get("_api") in (None, "classic") else 1, f["name"])
        for f in sorted(parsed["functions"], key=sortkey):
            sig = fmt_function(f)
            ex = f.get("executor")
            ex_s = f" executor={ex}" if ex else ""
            api = f.get("_api")
            api_s = f" **[carbon]**" if api == "carbon" else ""
            comment = clean_comment(f.get("comment"))
            c_s = f" — {comment}" if comment else ""
            out += [f"### {f['name']}  ", "",
                    f"```c\n{sig}\n```", "",
                    f"Trap: {fmt_trap(f.get('trap'))}{ex_s}{api_s}{c_s}", ""]
        out.append("")

    out += [type_section("Typedefs", parsed["typedefs"], None)]
    out += [type_section("Enums", parsed["enums"], None)]
    # enum values detail
    if parsed["enums"]:
        out += ["### Enum Values", ""]
        for e in parsed["enums"]:
            nm = e.get("name", "anonymous")
            comment = clean_comment(e.get("comment"))
            out += [f"**{nm}**{(' — ' + comment) if comment else ''}:", ""]
            for v in e.get("values", []):
                out += [f"- `{v.get('name','?')}` = {v.get('value','?')}"]
            out.append("")
    out += [type_section("Structs", parsed["structs"], None)]
    out += [type_section("Unions", parsed["unions"], None)]
    out += [type_section("Function Pointers", parsed["funptrs"], None)]
    out += [type_section("Common Blocks", parsed["commons"], None)]
    out += [type_section("Dispatchers", parsed["dispatchers"], None)]
    out += [type_section("Low Memory Globals", parsed["lowmems"], None)]
    import re
    md = re.sub(r"\n{3,}", "\n\n", "\n".join(out))
    return md

def main():
    files = sorted(f for f in os.listdir(DEFS_DIR) if f.endswith(".yaml"))
    os.makedirs(OUT_DIR, exist_ok=True)
    summary = []
    for fn in files:
        domain = fn[:-5]
        parsed = parse_file(os.path.join(DEFS_DIR, fn))
        if parsed is None:
            summary.append((domain, 0, 0, 0, 0))
            continue
        nf = len(parsed["functions"])
        nt = len(parsed["typedefs"]) + len(parsed["structs"]) + len(parsed["unions"])
        nc = len(parsed["enums"])
        nl = len(parsed["lowmems"])
        summary.append((domain, nf, nt, nc, nl))
        md = render_domain(domain, parsed)
        with open(os.path.join(OUT_DIR, f"{domain}.md"), "w", encoding="utf-8") as f:
            f.write(md)
        print(f"{domain}: {nf} funcs, {nt} types, {nc} enums, {nl} lowmem")

    total_f = sum(s[1] for s in summary)
    total_t = sum(s[2] for s in summary)
    total_c = sum(s[3] for s in summary)
    total_l = sum(s[4] for s in summary)

    # README index
    rows = "".join(
        f"| [{d}]({d}.md) | {nf} | {nt} | {nc} | {nl} |\n"
        for d, nf, nt, nc, nl in summary)
    readme = f"""# Multiversal Interfaces — API Reference

由 [Retro68](https://github.com/autc04/Retro68) 子模块 `multiversal`
（[autc04/multiversal](https://github.com/autc04/multiversal)）的
`defs/*.yaml` 自动生成。本目录共 **{len(summary)}** 个 API 域，
**{total_f}** 个函数、**{total_t}** 个类型、**{total_c}** 个枚举、**{total_l}** 个低内存全局变量。

## 这是什么

Multiversal Interfaces 是经典 Mac OS（Carbon 之前）Toolbox API 的开源重实现，
用 YAML 定义、由 Ruby 生成器转为 C/C++ 头文件（Retro68 编译器使用）。
来源是 Executor 2000 的干净室实现头文件（宽松许可证，可自由再分发）。

**覆盖范围**：System 7.0 及以前。**不包含**：Carbon、MacTCP、OpenTransport、
Navigation Services（仅少量定义）、System 7 之后引入的 API。

> 也可使用 Apple Universal Interfaces（需自备，来自 MPW/CodeWarrior），
> 覆盖更全（含 Carbon）。两者二选一，构建时自动检测。

## 使用

Retro68 编译时 `-I` 已指向生成的头文件。直接 `#include <Quickdraw.h>` 等即可。
函数带 `trap` 的（如 `CopyBits` = `0xA8EC`）由 Retro68 生成 trap 调用胶水；
`executor: C_` 表示函数体在模拟器/运行时实现。

## API 域索引

| 域 | 函数 | 类型 | 枚举 | 低内存 |
|---|---|---|---|---|
{rows}
## 相关

- 生成脚本：`tools/gen-multiversal-docs.py`
- 上游仓库：`ref/Retro68/multiversal`（submodule，commit `ac0a295`）
"""
    with open(os.path.join(OUT_DIR, "README.md"), "w", encoding="utf-8") as f:
        f.write(readme)
    print(f"\nTotal: {len(summary)} domains, {total_f} funcs, {total_t} types, "
          f"{total_c} enums, {total_l} lowmem -> {OUT_DIR}")

if __name__ == "__main__":
    main()
