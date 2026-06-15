#!/usr/bin/env python3
"""
project_summarizer.py — Interactive project documentation generator.

Generates plain-text (human-friendly) and LLM-optimized Markdown reports:
  - Git commit history (hash, date, author, full message)
  - Directory structure (tree view)
  - Project overview (from README, requirements, etc.)
  - Code analysis (function/class signatures, import graph, configs)
"""

import os
import re
import sys
import ast
import subprocess
import xml.sax.saxutils
from datetime import datetime
from pathlib import Path
from collections import Counter, defaultdict

try:
    import tiktoken
    _HAS_TIKTOKEN = True
except ImportError:
    _HAS_TIKTOKEN = False

try:
    from rich.console import Console
    from rich.panel import Panel
    from rich.progress import Progress, SpinnerColumn, TextColumn
    from rich.rule import Rule
except ImportError:
    print("This script requires 'rich'. Install with: pip install rich")
    sys.exit(1)

try:
    import questionary
except ImportError:
    print("This script requires 'questionary'. Install with: pip install questionary")
    sys.exit(1)

console = Console()
RS = "\x1e"
US = "\x1f"

# ═══════════════════════════════════════════════════════════
# DATA EXTRACTION — CORE
# ═══════════════════════════════════════════════════════════

def check_git_repo(path="."):
    try:
        subprocess.run(["git", "rev-parse", "--git-dir"], cwd=path, capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def get_git_commits(max_count=None, path="."):
    cmd = ["git", "log", "--all", f"--format={RS}%H{US}%ai{US}%an{US}%B{RS}", "--no-color"]
    if max_count is not None:
        cmd.extend(["--max-count", str(max_count)])
    result = subprocess.run(cmd, cwd=path, capture_output=True, text=True)
    if result.returncode != 0:
        console.print("[red]Failed to retrieve git log.[/red]")
        return []
    commits = []
    for record in result.stdout.split(RS):
        record = record.strip()
        if not record:
            continue
        fields = record.split(US)
        if len(fields) >= 4:
            commits.append({
                "hash": fields[0][:7],
                "date": fields[1],
                "author": fields[2],
                "message": fields[3].strip(),
            })
    return commits


def build_tree_dict(root_dir, exclude_dirs=None):
    if exclude_dirs is None:
        exclude_dirs = {".git", "__pycache__", ".venv", "venv", ".egg-info",
                        "node_modules", ".mypy_cache", ".pytest_cache", ".tox", ".idea", ".vscode"}
    root = Path(root_dir).resolve()
    tree = {}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in exclude_dirs]
        rel = Path(dirpath).relative_to(root)
        current = tree
        if rel != Path("."):
            for part in rel.parts:
                current = current[part]
        for d in sorted(dirnames):
            current[d] = {}
        for f in sorted(filenames):
            current[f] = None
    return tree


def render_tree_text(tree, root_name="", prefix=""):
    lines = []
    if root_name:
        lines.append(root_name + "/")
    items = list(tree.items())
    for i, (name, subtree) in enumerate(items):
        is_last = i == len(items) - 1
        conn = "└── " if is_last else "├── "
        display = name + "/" if isinstance(subtree, dict) else name
        lines.append(prefix + conn + display)
        if isinstance(subtree, dict) and subtree:
            ext = "    " if is_last else "│   "
            lines.extend(render_tree_text(subtree, "", prefix + ext))
    return lines


def get_file_stats(root_dir):
    root = Path(root_dir).resolve()
    ext_counts = Counter()
    total_lines = 0
    total_files = 0
    skip = {".git", "__pycache__", ".venv", "venv", ".egg-info", "node_modules", ".mypy_cache", ".pytest_cache"}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in skip]
        for f in filenames:
            total_files += 1
            ext = Path(f).suffix or "(no ext)"
            ext_counts[ext] += 1
            try:
                total_lines += sum(1 for _ in open(Path(dirpath) / f, "r", encoding="utf-8", errors="ignore"))
            except Exception:
                pass
    return total_files, total_lines, ext_counts


def parse_readme(root_dir):
    for p in Path(root_dir).glob("README*"):
        sections = defaultdict(str)
        current = "general"
        content = p.read_text(encoding="utf-8", errors="ignore")
        title = re.search(r"^# (.+)$", content, re.MULTILINE)
        title = title.group(1).strip() if title else ""
        for line in content.split("\n"):
            m = re.match(r"^## (.+)$", line)
            if m:
                current = m.group(1).strip().lower()
            else:
                sections[current] += line + "\n"
        for k in sections:
            sections[k] = sections[k].strip()
        result = {"title": title}
        result.update(sections)
        return result
    return {}


def parse_requirements(root_dir):
    for fname in ("requirements.txt", "requirements.in"):
        p = Path(root_dir) / fname
        if p.exists():
            deps = []
            for line in p.read_text(encoding="utf-8", errors="ignore").split("\n"):
                line = line.strip()
                if line and not line.startswith("#") and not line.startswith("-"):
                    deps.append(line)
            return deps
    return []


LANG_MAP = {
    ".py": "Python", ".js": "JavaScript", ".ts": "TypeScript",
    ".tsx": "TypeScript React", ".jsx": "JavaScript React",
    ".java": "Java", ".cpp": "C++", ".c": "C", ".h": "C/C++ Header",
    ".go": "Go", ".rs": "Rust", ".rb": "Ruby", ".php": "PHP",
    ".swift": "Swift", ".kt": "Kotlin", ".scala": "Scala",
    ".cs": "C#", ".fs": "F#", ".r": "R", ".m": "MATLAB",
    ".sql": "SQL", ".html": "HTML", ".css": "CSS",
    ".scss": "SCSS", ".sh": "Shell", ".ps1": "PowerShell",
    ".yaml": "YAML", ".yml": "YAML", ".json": "JSON",
    ".xml": "XML", ".toml": "TOML", ".ini": "INI",
    ".md": "Markdown", ".rst": "reStructuredText",
    ".dockerfile": "Dockerfile", ".tf": "Terraform", ".txt": "Text",
}

CONFIG_FILE_NAMES = {
    "pyproject.toml", "package.json", "Cargo.toml", "go.mod",
    "tsconfig.json", "Dockerfile", "docker-compose.yml",
    "Makefile", "Justfile", ".env.example", ".gitignore",
    ".editorconfig", ".prettierrc", "eslint.config.js",
    "composer.json", "Gemfile", "Podfile",
}


def get_project_summary(root_dir):
    root = Path(root_dir).resolve()
    total_files, total_lines, ext_counts = get_file_stats(root_dir)
    readme_data = parse_readme(root_dir)
    filtered = {e: c for e, c in ext_counts.items() if e in LANG_MAP}
    primary = LANG_MAP.get(max(filtered, key=filtered.get), "") if filtered else ""
    deps = parse_requirements(root_dir)
    return {
        "project_name": root.name,
        "total_files": total_files,
        "total_lines": total_lines,
        "primary_language": primary,
        "description": readme_data.get("general", "").strip() or readme_data.get("title", ""),
        "features": readme_data.get("features", ""),
        "usage": readme_data.get("usage", ""),
        "installation": readme_data.get("installation", ""),
        "dependencies": deps,
        "file_extensions": dict(sorted(ext_counts.items(), key=lambda x: -x[1])),
    }


# ═══════════════════════════════════════════════════════════
# DATA EXTRACTION — LLM ENHANCEMENTS
# ═══════════════════════════════════════════════════════════

def extract_source_signatures(root_dir):
    root = Path(root_dir).resolve()
    signatures = {}
    skip = {".git", "__pycache__", ".venv", "venv", ".egg-info", "node_modules"}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in skip]
        for f in filenames:
            if not f.endswith(".py"):
                continue
            fp = Path(dirpath) / f
            rel = str(fp.relative_to(root).as_posix())
            try:
                tree = ast.parse(fp.read_text(encoding="utf-8", errors="ignore"))
            except SyntaxError:
                continue
            functions = []
            classes = []
            imports = []
            for node in ast.iter_child_nodes(tree):
                if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                    doc = ast.get_docstring(node) or ""
                    first_doc = doc.split("\n")[0] if doc else ""
                    functions.append({
                        "name": node.name,
                        "lineno": node.lineno,
                        "decorators": [d.id if isinstance(d, ast.Name) else d.attr if isinstance(d, ast.Attribute) else "" for d in node.decorator_list],
                        "docstring": first_doc,
                    })
                elif isinstance(node, ast.ClassDef):
                    doc = ast.get_docstring(node) or ""
                    first_doc = doc.split("\n")[0] if doc else ""
                    methods = []
                    for item in ast.iter_child_nodes(node):
                        if isinstance(item, (ast.FunctionDef, ast.AsyncFunctionDef)):
                            mdoc = ast.get_docstring(item) or ""
                            methods.append({
                                "name": item.name,
                                "lineno": item.lineno,
                                "decorators": [d.id if isinstance(d, ast.Name) else d.attr if isinstance(d, ast.Attribute) else "" for d in item.decorator_list],
                                "docstring": mdoc.split("\n")[0] if mdoc else "",
                            })
                    classes.append({
                        "name": node.name,
                        "lineno": node.lineno,
                        "decorators": [d.id if isinstance(d, ast.Name) else d.attr if isinstance(d, ast.Attribute) else "" for d in node.decorator_list],
                        "docstring": first_doc,
                        "methods": methods,
                    })
                elif isinstance(node, ast.Import):
                    for alias in node.names:
                        imports.append(alias.name)
                elif isinstance(node, ast.ImportFrom):
                    module = node.module or ""
                    for alias in node.names:
                        imports.append(f"{module}.{alias.name}" if module else alias.name)
            if functions or classes or imports:
                with open(fp, "r", encoding="utf-8", errors="ignore") as fh:
                    line_count = sum(1 for _ in fh)
                signatures[rel] = {
                    "functions": functions,
                    "classes": classes,
                    "imports": imports,
                    "lines": line_count,
                }
    return signatures


def extract_import_graph(root_dir):
    root = Path(root_dir).resolve()
    graph = {}
    skip = {".git", "__pycache__", ".venv", "venv", ".egg-info", "node_modules"}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in skip]
        for f in filenames:
            if not f.endswith(".py"):
                continue
            fp = Path(dirpath) / f
            rel = str(fp.relative_to(root).as_posix())
            try:
                tree = ast.parse(fp.read_text(encoding="utf-8", errors="ignore"))
            except SyntaxError:
                continue
            local_imports = set()
            external_imports = set()
            for node in ast.iter_child_nodes(tree):
                if isinstance(node, ast.Import):
                    for alias in node.names:
                        top = alias.name.split(".")[0]
                        if (root / (top + ".py")).exists() or (root / top).is_dir():
                            local_imports.add(alias.name)
                        else:
                            external_imports.add(top)
                elif isinstance(node, ast.ImportFrom):
                    mod = node.module or ""
                    if mod:
                        top = mod.split(".")[0]
                        if (root / (top + ".py")).exists() or (root / top).is_dir():
                            local_imports.add(mod)
                        else:
                            external_imports.add(top)
            graph[rel] = {
                "local": sorted(local_imports),
                "external": sorted(external_imports),
            }
    return graph


def collect_config_files(root_dir):
    root = Path(root_dir).resolve()
    configs = {}
    for fname in CONFIG_FILE_NAMES:
        fp = root / fname
        if fp.exists() and fp.is_file():
            try:
                content = fp.read_text(encoding="utf-8", errors="ignore")
                configs[fname] = content
            except Exception:
                pass
    return configs


def generate_architecture_summary(project_name, signatures, graph, summary):
    parts = []
    parts.append(f"Project '{project_name}' is a {summary.get('primary_language', '')} project".strip() + ".")
    total_src = len(signatures)
    total_funcs = sum(len(s["functions"]) for s in signatures.values())
    total_classes = sum(len(s["classes"]) for s in signatures.values())
    if total_src > 0:
        parts.append(f"Contains {total_src} source file(s) with {total_funcs} function(s) and {total_classes} class(es).")
    else:
        parts.append("Contains no Python source files with parseable definitions.")
    if graph:
        files_with_imports = sum(1 for v in graph.values() if v["local"] or v["external"])
        ext_deps = set()
        for v in graph.values():
            ext_deps.update(v["external"])
        parts.append(f"{files_with_imports} file(s) have imports, referencing {len(ext_deps)} external package(s).")
    deps = summary.get("dependencies", [])
    if deps:
        parts.append(f"Project declares {len(deps)} dependency/dependencies: {', '.join(deps)}.")
    total_files = summary.get("total_files", 0)
    total_lines = summary.get("total_lines", 0)
    parts.append(f"Repository has {total_files} total file(s) and ~{total_lines} line(s) of code.")
    return " ".join(parts)


FILE_CONTENT_MAX_LINES = 200
FILE_CONTENT_MAX_KB = 100


def get_file_contents(root_dir, output_dir=None):
    root = Path(root_dir).resolve()
    output_dir = Path(output_dir).resolve() if output_dir else None
    contents = {}
    skip_exts = {".pyc", ".pyo", ".exe", ".dll", ".so", ".dylib", ".bin", ".png", ".jpg", ".jpeg", ".gif", ".ico", ".svg", ".woff", ".woff2", ".ttf", ".eot"}
    skip_dirs = {".git", "__pycache__", ".venv", "venv", ".egg-info", "node_modules", ".mypy_cache", ".pytest_cache", ".tox", ".idea", ".vscode"}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in skip_dirs]
        for f in filenames:
            fp = Path(dirpath) / f
            ext = fp.suffix.lower()
            if ext in skip_exts:
                continue
            rel = str(fp.relative_to(root).as_posix())
            # Skip files inside output_dir to avoid including previously generated reports
            if output_dir and output_dir in fp.parents:
                continue
            try:
                size = fp.stat().st_size
                if size > FILE_CONTENT_MAX_KB * 1024:
                    continue
                text = fp.read_text(encoding="utf-8", errors="ignore")
                line_count = text.count("\n") + 1
                if line_count > FILE_CONTENT_MAX_LINES:
                    text = "\n".join(text.split("\n")[:FILE_CONTENT_MAX_LINES]) + f"\n... (truncated at {FILE_CONTENT_MAX_LINES} lines)"
                # Escape CDATA closing sequence to prevent XML breakage
                text = text.replace("]]>", "]]]]><![CDATA[>")
                contents[rel] = text
            except Exception:
                pass
    return contents


def estimate_tokens(text):
    if not _HAS_TIKTOKEN:
        return None
    try:
        enc = tiktoken.get_encoding("cl100k_base")
        return len(enc.encode(text))
    except Exception:
        return None


# ═══════════════════════════════════════════════════════════
# OUTPUT GENERATION — HUMAN TEXT
# ═══════════════════════════════════════════════════════════

def generate_text_report(commits, tree, summary, signatures=None, import_graph=None, configs=None, arch_summary=None):
    lines = []
    sep = "=" * 72
    sub = "-" * 72
    name = summary.get("project_name", "Untitled")

    lines.append(sep)
    lines.append(f"  PROJECT REPORT : {name}")
    lines.append(f"  Generated      : {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(sep)
    lines.append("")

    # 1 — Commits
    lines.append("1.  GIT COMMITS")
    lines.append(sep)
    lines.append(f"    Total Commits: {len(commits)}")
    lines.append("")
    if commits:
        lines.append(f"  {'Hash':<10} {'Date':<27} {'Author':<20} Message")
        lines.append(f"  {'-'*9:<10} {'-'*26:<27} {'-'*19:<20} {'-'*60}")
        for c in commits:
            d = c["date"].replace("T", " ")[:19]
            msg = c["message"].split("\n")[0][:57]
            lines.append(f"  {c['hash']:<10} {d:<27} {c['author']:<20} {msg}")
        lines.append("")
        lines.append("  Full Messages:")
        lines.append(sub)
        for i, c in enumerate(commits, 1):
            lines.append(f"  [{i}] {c['hash']}  {c['date'][:19]}  {c['author']}")
            lines.append("  " + "-" * 68)
            for l in c["message"].split("\n"):
                lines.append(f"    {l}")
            lines.append("")
    else:
        lines.append("    (No commits found)")
        lines.append("")

    # 2 — Tree
    lines.append("2.  DIRECTORY STRUCTURE")
    lines.append(sep)
    lines.append("")
    lines.extend(render_tree_text(tree, name))
    lines.append("")

    # 3 — Summary
    lines.append("3.  PROJECT SUMMARY")
    lines.append(sep)
    lines.append("")
    lines.append(f"    Project Name     : {summary.get('project_name', 'N/A')}")
    lines.append(f"    Total Files      : {summary.get('total_files', 'N/A')}")
    lines.append(f"    Total Lines      : {summary.get('total_lines', 'N/A')}")
    lines.append(f"    Primary Language : {summary.get('primary_language') or 'N/A'}")
    lines.append("")
    for field in ("description", "features", "installation", "usage"):
        val = summary.get(field, "")
        if val:
            lines.append(f"    {field.capitalize()}:")
            for l in val.split("\n"):
                l = l.strip()
                if l:
                    lines.append(f"      {l}")
            lines.append("")
    deps = summary.get("dependencies", [])
    if deps:
        lines.append("    Dependencies:")
        for d in deps:
            lines.append(f"      - {d}")
        lines.append("")
    exts = summary.get("file_extensions", {})
    if exts:
        lines.append("    File Breakdown by Extension:")
        for ext, count in exts.items():
            lang = LANG_MAP.get(ext, "Other")
            lines.append(f"      {ext:<12} {count:<4} file(s)  ({lang})")
        lines.append("")

    # 4 — Code Analysis (LLM extras)
    if signatures or import_graph or configs or arch_summary:
        lines.append("4.  CODE ANALYSIS")
        lines.append(sep)
        lines.append("")

        if arch_summary:
            lines.append("    Architecture Overview:")
            lines.append(f"      {arch_summary}")
            lines.append("")

        if signatures:
            lines.append("    Function & Class Signatures:")
            lines.append(sub)
            for filepath in sorted(signatures):
                s = signatures[filepath]
                lines.append(f"    File: {filepath}  ({s['lines']} lines)")
                if s["functions"]:
                    for fn in s["functions"]:
                        deco = f" @{', @'.join(fn['decorators'])}" if fn["decorators"] else ""
                        doc = f"  // {fn['docstring']}" if fn["docstring"] else ""
                        lines.append(f"      ─ def {fn['name']}{deco}{doc}")
                if s["classes"]:
                    for cl in s["classes"]:
                        deco = f" @{', @'.join(cl['decorators'])}" if cl["decorators"] else ""
                        doc = f"  // {cl['docstring']}" if cl["docstring"] else ""
                        lines.append(f"      ─ class {cl['name']}{deco}{doc}")
                        for m in cl["methods"]:
                            mdeco = f" @{', @'.join(m['decorators'])}" if m["decorators"] else ""
                            mdoc = f"  // {m['docstring']}" if m["docstring"] else ""
                            lines.append(f"          ─ def {m['name']}{mdeco}{mdoc}")
            lines.append("")

        if import_graph:
            lines.append("    Import Dependency Graph:")
            lines.append(sub)
            for filepath in sorted(import_graph):
                g = import_graph[filepath]
                parts = []
                if g["local"]:
                    parts.append(f"local: {', '.join(g['local'])}")
                if g["external"]:
                    parts.append(f"external: {', '.join(g['external'])}")
                if parts:
                    lines.append(f"      {filepath}  ->  {'; '.join(parts)}")
                else:
                    lines.append(f"      {filepath}  (no imports)")
            lines.append("")

        if configs:
            lines.append("    Key Configuration Files:")
            lines.append(sub)
            for fname, content in sorted(configs.items()):
                lines.append(f"      === {fname} ===")
                for l in content.split("\n"):
                    lines.append(f"      {l}")
                lines.append("")

    lines.append(sep)
    lines.append(f"  Generated by project_summarizer.py")
    lines.append(sep)
    return "\n".join(lines)


# ═══════════════════════════════════════════════════════════
# OUTPUT GENERATION — LLM-OPTIMIZED MARKDOWN
# ═══════════════════════════════════════════════════════════

def generate_llm_markdown_report(commits, tree, summary, signatures=None, import_graph=None, configs=None, arch_summary=None):
    md = []
    name = summary.get("project_name", "Untitled")
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    md.append("# Project Report: " + name)
    md.append("")
    md.append(f"> Generated: {ts}  |  Format: LLM-optimized")
    md.append("")

    # ── SECTION 1: Identity ──
    md.append("## 1. Project Identity")
    md.append("")
    md.append("| Property | Value |")
    md.append("|----------|-------|")
    md.append(f"| **Name** | {name} |")
    md.append(f"| **Language** | {summary.get('primary_language') or 'N/A'} |")
    md.append(f"| **Total Files** | {summary.get('total_files', 'N/A')} |")
    md.append(f"| **Total Lines** | {summary.get('total_lines', 'N/A')} |")
    md.append(f"| **Dependencies** | {len(summary.get('dependencies', []))} |")
    md.append(f"| **Git Commits** | {len(commits)} |")
    md.append("")

    if arch_summary:
        md.append("### Architecture Overview")
        md.append("")
        md.append(arch_summary)
        md.append("")

    if summary.get("description"):
        md.append("### Description")
        md.append("")
        md.append(summary["description"])
        md.append("")

    if summary.get("dependencies"):
        md.append("### Dependencies")
        md.append("")
        for dep in summary["dependencies"]:
            md.append(f"- `{dep}`")
        md.append("")

    # ── SECTION 2: File Reference ──
    md.append("## 2. File Reference")
    md.append("")
    if signatures:
        md.append("| File | Lines | Functions | Classes | Imports |")
        md.append("|------|-------|-----------|---------|---------|")
        for fp in sorted(signatures):
            s = signatures[fp]
            fn_list = ", ".join(f"`{f['name']}`" for f in s["functions"])
            cl_list = ", ".join(f"`{c['name']}`" for c in s["classes"])
            imp_count = len(s["imports"])
            md.append(f"| `{fp}` | {s['lines']} | {fn_list or '—'} | {cl_list or '—'} | {imp_count} |")
        md.append("")
    else:
        md.append("*(No source analysis available)*")
        md.append("")

    # ── SECTION 3: Import Graph ──
    md.append("## 3. Import Dependency Graph")
    md.append("")
    if import_graph:
        for fp in sorted(import_graph):
            g = import_graph[fp]
            md.append(f"- **`{fp}`**")
            if g["local"]:
                for dep in g["local"]:
                    md.append(f"  - imports `{dep}` (local)")
            if g["external"]:
                for dep in g["external"]:
                    md.append(f"  - imports `{dep}` (external)")
            if not g["local"] and not g["external"]:
                md.append("  _no imports_")
        md.append("")
    else:
        md.append("*(No import analysis available)*")
        md.append("")

    # ── SECTION 4: Signatures ──
    md.append("## 4. Function & Class Signatures")
    md.append("")
    if signatures:
        for fp in sorted(signatures):
            s = signatures[fp]
            md.append(f"### `{fp}` ({s['lines']} lines)")
            md.append("")
            if s["functions"]:
                md.append("**Functions:**")
                md.append("")
                for fn in s["functions"]:
                    deco = " (" + ", ".join(f"@{d}" for d in fn["decorators"]) + ")" if fn["decorators"] else ""
                    doc = f" — {fn['docstring']}" if fn["docstring"] else ""
                    md.append(f"- `def {fn['name']}{deco}`{doc}")
                md.append("")
            if s["classes"]:
                md.append("**Classes:**")
                md.append("")
                for cl in s["classes"]:
                    deco = " (" + ", ".join(f"@{d}" for d in cl["decorators"]) + ")" if cl["decorators"] else ""
                    doc = f" — {cl['docstring']}" if cl["docstring"] else ""
                    md.append(f"- `class {cl['name']}{deco}`{doc}")
                    if cl["methods"]:
                        for m in cl["methods"]:
                            mdeco = " (" + ", ".join(f"@{d}" for d in m["decorators"]) + ")" if m["decorators"] else ""
                            mdoc = f" — {m['docstring']}" if m["docstring"] else ""
                            md.append(f"  - `def {m['name']}{mdeco}`{mdoc}")
                md.append("")
    else:
        md.append("*(No signature data)*")
        md.append("")

    # ── SECTION 5: Config Files ──
    if configs:
        md.append("## 5. Key Configuration Files")
        md.append("")
        for fname, content in sorted(configs.items()):
            ext = Path(fname).suffix
            lang = LANG_MAP.get(ext, "")
            md.append(f"### `{fname}`")
            md.append("")
            md.append(f"```{lang.lower() if lang else ''}")
            md.append(content.rstrip())
            md.append("```")
            md.append("")

    # ── SECTION 6: Directory Tree ──
    tree_sec = 6 if configs else 5
    md.append(f"## {tree_sec}. Directory Structure")
    md.append("")
    md.append("```")
    md.extend(render_tree_text(tree, name))
    md.append("```")
    md.append("")

    # ── SECTION 7: Git History ──
    git_sec = tree_sec + 1
    md.append(f"## {git_sec}. Git History")
    md.append("")
    md.append(f"- **Total Commits:** {len(commits)}")
    md.append("")
    if commits:
        md.append("| Hash | Date | Author | Message |")
        md.append("|------|------|--------|---------|")
        for c in commits:
            d = c["date"].replace("T", " ")[:19]
            msg = c["message"].split("\n")[0].replace("|", "\\|")
            md.append(f"| `{c['hash']}` | {d} | {c['author']} | {msg} |")
        md.append("")
        for i, c in enumerate(commits, 1):
            first = c["message"].split("\n")[0]
            md.append(f"<details>")
            md.append(f"<summary><b>Commit {i}:</b> <code>{c['hash']}</code> — {first}</summary>")
            md.append("")
            md.append("```")
            md.append(f"Hash:   {c['hash']}")
            md.append(f"Date:   {c['date']}")
            md.append(f"Author: {c['author']}")
            md.append("")
            md.append(c["message"])
            md.append("```")
            md.append("</details>")
            md.append("")
    else:
        md.append("*(No commits found)*")
        md.append("")

    md.append("---")
    md.append("")
    md.append(f"*Report generated by `project_summarizer.py` on {ts}*")
    return "\n".join(md)


# ═══════════════════════════════════════════════════════════
# OUTPUT GENERATION — PROMPT-OPTIMIZED XML
# ═══════════════════════════════════════════════════════════

def _xc(text):
    """XML-escape a string for use in attributes/text content."""
    return xml.sax.saxutils.escape(text)


def generate_xml_report(commits, tree, summary, signatures=None, import_graph=None,
                         configs=None, arch_summary=None, file_contents=None):
    lines = []
    name = summary.get("project_name", "Untitled")
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    lines.append('<?xml version="1.0" encoding="UTF-8"?>')
    lines.append("<project_report>")
    lines.append("")

    # ── Instructions ──
    lines.append("  <instructions>")
    lines.append("    <![CDATA[")
    lines.append(f"    You are analyzing a software project called '{_xc(name)}'.")
    lines.append("    The XML below provides structured context: <directory_tree> for layout,")
    lines.append("    <file_contents> for actual source code, and <code_analysis> for")
    lines.append("    function/class signatures and dependency relationships.")
    lines.append("    Use this information to understand, debug, refactor, or generate code for the project.")
    lines.append("    ]]>")
    lines.append("  </instructions>")
    lines.append("")

    # ── Identity ──
    lines.append("  <project_identity>")
    lines.append(f"    <name>{_xc(name)}</name>")
    lines.append(f"    <language>{_xc(summary.get('primary_language', 'N/A'))}</language>")
    lines.append(f"    <total_files>{summary.get('total_files', 'N/A')}</total_files>")
    lines.append(f"    <total_lines>{summary.get('total_lines', 'N/A')}</total_lines>")
    lines.append(f"    <dependencies>{len(summary.get('dependencies', []))}</dependencies>")
    lines.append(f"    <git_commits>{len(commits)}</git_commits>")
    if summary.get("dependencies"):
        lines.append("    <dependency_list>")
        for dep in summary["dependencies"]:
            lines.append(f"      <dep>{_xc(dep)}</dep>")
        lines.append("    </dependency_list>")
    lines.append("  </project_identity>")
    lines.append("")

    if arch_summary:
        lines.append("  <architecture_overview>")
        lines.append(f"    {_xc(arch_summary)}")
        lines.append("  </architecture_overview>")
        lines.append("")

    if summary.get("description"):
        lines.append("  <description>")
        lines.append(f"    <![CDATA[{summary['description']}]]>")
        lines.append("  </description>")
        lines.append("")

    # ── Directory Tree ──
    lines.append("  <directory_tree>")
    lines.append("    <![CDATA[")
    for tl in render_tree_text(tree, name):
        lines.append("    " + tl)
    lines.append("    ]]>")
    lines.append("  </directory_tree>")
    lines.append("")

    # ── File Contents ──
    if file_contents:
        lines.append("  <file_contents>")
        for fpath in sorted(file_contents):
            raw = file_contents[fpath]
            ext = Path(fpath).suffix
            lang = LANG_MAP.get(ext, "")
            lc = raw.count("\n") + 1
            lines.append(f'    <file path="{_xc(fpath)}" language="{_xc(lang)}" lines="{lc}">')
            lines.append("      <![CDATA[")
            for cl in raw.split("\n"):
                lines.append("      " + cl)
            lines.append("      ]]>")
            lines.append("    </file>")
        lines.append("  </file_contents>")
        lines.append("")

    # ── Code Analysis ──
    has_analysis = any([signatures, import_graph, configs])
    if has_analysis:
        lines.append("  <code_analysis>")
        if signatures:
            lines.append("    <signatures>")
            for fp in sorted(signatures):
                s = signatures[fp]
                def _mkdec(dlist):
                    return _xc(", ".join(dlist)) if dlist else ""
                def _mkdoc(doc):
                    return _xc(doc) if doc else ""

                lines.append(f'      <file path="{_xc(fp)}" lines="{s["lines"]}">')
                for fn in s["functions"]:
                    a_deco = f' decorators="{_mkdec(fn["decorators"])}"' if fn["decorators"] else ""
                    a_doc = f' docstring="{_mkdoc(fn["docstring"])}"' if fn["docstring"] else ""
                    lines.append(f'        <function name="{_xc(fn["name"])}" lineno="{fn["lineno"]}"{a_deco}{a_doc}/>')
                for cl in s["classes"]:
                    a_deco = f' decorators="{_mkdec(cl["decorators"])}"' if cl["decorators"] else ""
                    a_doc = f' docstring="{_mkdoc(cl["docstring"])}"' if cl["docstring"] else ""
                    lines.append(f'        <class name="{_xc(cl["name"])}" lineno="{cl["lineno"]}"{a_deco}{a_doc}>')
                    for m in cl["methods"]:
                        a_mdeco = f' decorators="{_mkdec(m["decorators"])}"' if m["decorators"] else ""
                        a_mdoc = f' docstring="{_mkdoc(m["docstring"])}"' if m["docstring"] else ""
                        lines.append(f'          <method name="{_xc(m["name"])}" lineno="{m["lineno"]}"{a_mdeco}{a_mdoc}/>')
                    lines.append("        </class>")
                lines.append(f"      </file>")
            lines.append("    </signatures>")
            lines.append("")

        if import_graph:
            lines.append("    <import_graph>")
            for fp in sorted(import_graph):
                g = import_graph[fp]
                lines.append(f'      <file path="{_xc(fp)}">')
                for dep in g["local"]:
                    lines.append(f'        <import_local>{_xc(dep)}</import_local>')
                for dep in g["external"]:
                    lines.append(f'        <import_external>{_xc(dep)}</import_external>')
                lines.append("      </file>")
            lines.append("    </import_graph>")
            lines.append("")

        if configs:
            lines.append("    <config_files>")
            for fname, content in sorted(configs.items()):
                lines.append(f'      <file name="{_xc(fname)}">')
                lines.append("        <![CDATA[")
                for cl in content.split("\n"):
                    lines.append("        " + cl)
                lines.append("        ]]>")
                lines.append("      </file>")
            lines.append("    </config_files>")
            lines.append("")

        lines.append("  </code_analysis>")
        lines.append("")

    # ── Git History ──
    lines.append("  <git_history>")
    lines.append(f"    <total_commits>{len(commits)}</total_commits>")
    lines.append("")
    if commits:
        lines.append("    <![CDATA[")
        for c in commits:
            d = c["date"].replace("T", " ")[:19]
            first = c["message"].split("\n")[0]
            lines.append(f"    {c['hash']}  {d}  {c['author']}  {first}")
        lines.append("    ]]>")
        lines.append("")
        for i, c in enumerate(commits, 1):
            lines.append(f"    <commit index=\"{i}\" hash=\"{c['hash']}\" author=\"{_xc(c['author'])}\" date=\"{c['date']}\">")
            lines.append("      <![CDATA[")
            for cl in c["message"].split("\n"):
                lines.append("      " + cl)
            lines.append("      ]]>")
            lines.append("    </commit>")
    else:
        lines.append("    <![CDATA[    (No commits found) ]]>")
    lines.append("  </git_history>")
    lines.append("")

    lines.append("</project_report>")
    return "\n".join(lines)


# ═══════════════════════════════════════════════════════════
# INTERACTIVE CLI
# ═══════════════════════════════════════════════════════════

def show_welcome():
    console.print()
    console.print(Panel.fit(
        "[bold cyan]  Project Summarizer  [/bold cyan]\n"
        "[white]Generate structured reports of your Git projects[/white]",
        border_style="bright_blue", padding=(1, 4),
    ))
    console.print()


def get_config():
    console.print("[bold yellow]Configure your report (arrow keys, spacebar to toggle)[/bold yellow]")
    console.print()

    max_raw = questionary.text(
        "Max commits (Enter for all):",
        default="",
        qmark="",
    ).ask()
    max_commits = int(max_raw) if max_raw and max_raw.strip() else None

    sections = questionary.checkbox(
        "Report sections:",
        choices=[
            questionary.Choice("Git Commits", value="commits", checked=True),
            questionary.Choice("Directory Tree", value="tree", checked=True),
            questionary.Choice("Project Summary", value="summary", checked=True),
            questionary.Choice("LLM Code Analysis", value="llm_extras", checked=True),
        ],
        qmark="",
    ).ask() or []

    output_dir_raw = questionary.text(
        "Output folder:",
        default="Repo_Report",
        qmark="",
    ).ask()
    output_dir = Path(output_dir_raw.strip() or "Repo_Report")

    llm_extras = []
    if "llm_extras" in sections:
        llm_extras = questionary.checkbox(
            "LLM code analysis details:",
            choices=[
                questionary.Choice("Function/Class Signatures", value="signatures", checked=True),
                questionary.Choice("Import Dependency Graph", value="import_graph", checked=True),
                questionary.Choice("Key Config Files", value="configs", checked=False),
                questionary.Choice("Architecture Summary", value="arch_summary", checked=True),
                questionary.Choice("Include File Contents", value="file_contents", checked=True),
            ],
            qmark="",
        ).ask() or []

    prefix = questionary.text(
        "Output filename prefix:",
        default="project_report",
        qmark="",
    ).ask()

    return {
        "max_commits": max_commits,
        "output_dir": output_dir,
        "sections": sections,
        "llm_extras": llm_extras,
        "prefix": prefix,
    }


def run_with_spinner(description, func, *args, **kwargs):
    with Progress(SpinnerColumn(), TextColumn("[progress.description]{task.description}"), transient=True) as prog:
        task = prog.add_task(f"[cyan]{description}...", total=None)
        result = func(*args, **kwargs)
        prog.update(task, description=f"[green]{description}... done!")
        return result


def show_data_overview(config, commits, tree, summary, llm_data):
    console.print()
    console.print(Rule("[bold blue]Extracted Data Overview[/bold blue]"))
    console.print()
    if "commits" in config["sections"]:
        console.print(f"  [bold]Commits:[/bold] {len(commits)} total")
    if "tree" in config["sections"]:
        console.print(f"  [bold]Tree:[/bold] built with {summary.get('total_files', '?')} files")
    if "summary" in config["sections"]:
        console.print(f"  [bold]Project:[/bold] {summary.get('project_name', 'N/A')}")
    if "llm_extras" in config["sections"]:
        sig_count = len(llm_data.get("signatures", {}))
        graph_count = len(llm_data.get("import_graph", {}))
        config_count = len(llm_data.get("configs", {}))
        filec_count = len(llm_data.get("file_contents", {}))
        extras = []
        if sig_count:
            extras.append(f"{sig_count} file(s) parsed")
        if graph_count:
            extras.append(f"{graph_count} file(s) in import graph")
        if config_count:
            extras.append(f"{config_count} config file(s)")
        if filec_count:
            extras.append(f"{filec_count} file(s) with full contents")
        if extras:
            console.print(f"  [bold]LLM Analysis:[/bold] {', '.join(extras)}")
    console.print()


def show_preview(text_content, md_llm_content, xml_content=None):
    console.print()
    console.print(Rule("[bold green]Preview — Plain Text (human)[/bold green]"))
    console.print()
    t_lines = text_content.split("\n")
    for line in t_lines[:20]:
        console.print(line)
    if len(t_lines) > 20:
        console.print(f"[dim]... ({len(t_lines) - 20} more lines) [/dim]")
    console.print()

    console.print(Rule("[bold green]Preview — LLM Markdown[/bold green]"))
    console.print()
    m_lines = md_llm_content.split("\n")
    for line in m_lines[:20]:
        console.print(line)
    if len(m_lines) > 20:
        console.print(f"[dim]... ({len(m_lines) - 20} more lines) [/dim]")
    console.print()

    if xml_content:
        console.print(Rule("[bold green]Preview — XML (prompt-optimized)[/bold green]"))
        console.print()
        x_lines = xml_content.split("\n")
        for line in x_lines[:20]:
            console.print(line)
        if len(x_lines) > 20:
            console.print(f"[dim]... ({len(x_lines) - 20} more lines) [/dim]")
        console.print()


def _resolve_path(path, label):
    """If file exists, ask overwrite or auto-number. Return final Path."""
    if not path.exists():
        return path
    console.print(f"[yellow]{label} already exists: {path.name}[/yellow]")
    ow = questionary.confirm("Overwrite?", default=False, qmark="").ask()
    if ow:
        return path
    parent = path.parent
    stem = path.stem
    ext = path.suffix
    i = 1
    while (parent / f"{stem}_{i}{ext}").exists():
        i += 1
    new_path = parent / f"{stem}_{i}{ext}"
    console.print(f"  Using [cyan]{new_path.name}[/cyan] instead.")
    return new_path


def feedback_loop(text_content, md_llm_content, xml_content, output_dir, prefix):
    pairs = [("text", ".txt", text_content), ("markdown", ".md", md_llm_content)]
    if xml_content:
        pairs.append(("xml", ".xml", xml_content))

    console.print()
    console.print(Rule("[bold yellow]Finalization[/bold yellow]"))
    console.print()

    fmt = questionary.select(
        "Which format to save?",
        choices=[
            questionary.Choice("Plain Text (human-friendly)", value="text"),
            questionary.Choice("Markdown (LLM-optimized)", value="markdown"),
            questionary.Choice("XML (prompt-optimized)", value="xml"),
            questionary.Choice("All three formats", value="all"),
        ],
        default="all" if xml_content else "markdown",
        qmark="",
    ).ask()

    changes = questionary.text(
        "Any specific features or formatting to change? (Enter = none):",
        default="",
        qmark="",
    ).ask()

    changes_display = changes if changes else "(none)"
    console.print(Panel(
        f"[bold]Format:[/bold] {fmt}\n"
        f"[bold]Changes requested:[/bold] {changes_display}",
        title="Feedback Summary",
        border_style="yellow",
    ))

    chosen = {"text", "markdown", "xml"} if fmt == "all" else {fmt}

    saved = []
    for key, ext, content in pairs:
        if key not in chosen:
            continue
        path = _resolve_path(output_dir / f"{prefix}{ext}", key.upper())
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        console.print(f"[green]Saved:[/green] {path}")
        saved.append(path)

    console.print()
    console.print(Panel.fit("[bold green]Report generation complete![/bold green]", border_style="green"))

    if saved:
        open_file = questionary.confirm(
            "Open the file to view?",
            default=False,
            qmark="",
        ).ask()
        if open_file:
            for fp in saved:
                try:
                    os.startfile(str(fp.resolve()))
                except Exception as e:
                    console.print(f"[red]Could not open {fp}: {e}[/red]")


# ═══════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════

def main():
    show_welcome()

    if not check_git_repo():
        console.print("[red]Not a git repository. Exiting.[/red]")
        sys.exit(1)

    config = get_config()
    sys.stdout.flush()

    # ── Press Enter to continue ──
    questionary.text("Press Enter to generate the report...", default="", qmark="").ask()
    console.print()

    # ── Create output folder ──
    output_dir = config["output_dir"]
    output_dir.mkdir(parents=True, exist_ok=True)

    # ── Extract base data ──
    commits = []
    if "commits" in config["sections"]:
        commits = run_with_spinner("Extracting commits", get_git_commits, max_count=config["max_commits"])

    tree = {}
    if "tree" in config["sections"]:
        tree = run_with_spinner("Building directory tree", build_tree_dict, ".")

    summary = {}
    if "summary" in config["sections"]:
        summary = run_with_spinner("Analyzing project", get_project_summary, ".")

    # ── Extract LLM extras ──
    llm_data = {"signatures": {}, "import_graph": {}, "configs": {}, "arch_summary": "", "file_contents": {}}
    if "llm_extras" in config["sections"]:
        if "signatures" in config["llm_extras"]:
            llm_data["signatures"] = run_with_spinner("Extracting source signatures", extract_source_signatures, ".")
        if "import_graph" in config["llm_extras"]:
            llm_data["import_graph"] = run_with_spinner("Building import graph", extract_import_graph, ".")
        if "configs" in config["llm_extras"]:
            llm_data["configs"] = run_with_spinner("Collecting config files", collect_config_files, ".")
        if "file_contents" in config["llm_extras"]:
            llm_data["file_contents"] = run_with_spinner("Reading file contents", get_file_contents, ".", config["output_dir"])
        if "arch_summary" in config["llm_extras"]:
            llm_data["arch_summary"] = generate_architecture_summary(
                summary.get("project_name", "project"),
                llm_data["signatures"],
                llm_data["import_graph"],
                summary,
            )

    show_data_overview(config, commits, tree, summary, llm_data)

    # ── Condition helpers ──
    _sig = llm_data["signatures"] if "signatures" in config.get("llm_extras", []) else None
    _ig = llm_data["import_graph"] if "import_graph" in config.get("llm_extras", []) else None
    _cfg = llm_data["configs"] if "configs" in config.get("llm_extras", []) else None
    _arch = llm_data["arch_summary"] if "arch_summary" in config.get("llm_extras", []) else None
    _fc = llm_data["file_contents"] if "file_contents" in config.get("llm_extras", []) else None

    # ── Generate outputs ──
    text_content = generate_text_report(commits, tree, summary, signatures=_sig, import_graph=_ig, configs=_cfg, arch_summary=_arch)

    md_llm_content = generate_llm_markdown_report(commits, tree, summary, signatures=_sig, import_graph=_ig, configs=_cfg, arch_summary=_arch)

    xml_content = None
    if _sig or _ig or _cfg or _fc:
        xml_content = generate_xml_report(commits, tree, summary, signatures=_sig, import_graph=_ig, configs=_cfg, arch_summary=_arch, file_contents=_fc)

    # ── Token estimate ──
    for label, content in [("Markdown", md_llm_content), ("XML", xml_content)]:
        if content:
            t = estimate_tokens(content)
            if t is not None:
                console.print(f"  [dim]{label} token estimate: ~{t:,} tokens (cl100k_base)[/dim]")
    console.print()

    show_preview(text_content, md_llm_content, xml_content)

    feedback_loop(text_content, md_llm_content, xml_content, output_dir, config["prefix"])


if __name__ == "__main__":
    main()
