#!/bin/sh
# ============================================================================
# list_repo_hashes.sh - 列举主仓库与所有第三方子模块的当前 HEAD 哈希
#
# 用途:
#   主要在内网机器上运行: 生成内网当前的基线 JSON 文件，把它拷贝到外网，
#   覆盖外网仓库的 scripts/bundle_baseline.json，然后在外网运行
#   scripts/export_bundles.sh 即可导出内网所需的增量 bundle。
#
# 用法:
#   ./list_repo_hashes.sh [输出JSON路径]
#       输出路径缺省为当前工作目录下的 bundle_baseline.json
#
# 输出:
#   1. 终端打印各仓库哈希一览表（短哈希 + 分支 + 状态备注）
#   2. 生成与 scripts/bundle_baseline.json 同格式的 JSON（完整 40 位哈希），
#      每行一个条目
#
# 说明:
#   - 未初始化的子模块哈希记为 ""，外网导出时将对它全量导出。
#   - 若子模块 HEAD 与主仓库记录的指针不一致，表格备注中会给出提示，
#     应先把指针更新提交到主仓库，否则内网同步时子模块不会跟进。
# ============================================================================

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd) || exit 1
ROOT=$(cd "$SCRIPT_DIR/.." && pwd) || exit 1
MAIN_NAME="data-workbench"
OUT_FILE="${1:-$PWD/bundle_baseline.json}"

info() { printf '%s\n' "$*"; }
warn() { printf '[警告] %s\n' "$*" >&2; }
err()  { printf '[错误] %s\n' "$*" >&2; }

if [ -z "$(command -v git)" ]; then
    err "未找到 git 命令"
    exit 1
fi

# 防误操作: 输出文件恰好是本仓库的基线文件时给出提醒
# （在外网机器上直接覆盖会使所有仓库看起来"无变更"）
if [ "$OUT_FILE" = "$SCRIPT_DIR/bundle_baseline.json" ]; then
    warn "输出文件就是本仓库的 scripts/bundle_baseline.json"
    warn "若本机是外网（导出）机器，这样覆盖会导致下次导出看不到任何变更"
    warn "若本机是内网（同步）机器，且刚应用完 bundle，则覆盖是正确的"
fi

REPO_LIST=$(mktemp) || exit 1
JSON_TMP=$(mktemp) || exit 1
trap 'rm -f "$REPO_LIST" "$JSON_TMP"' EXIT INT TERM

# ---------------------------------------------------------------------------
# 仓库清单: 每行 "名称|相对路径"，主仓库在前
# ---------------------------------------------------------------------------
{
    printf '%s|%s\n' "$MAIN_NAME" "."
    git config --file "$ROOT/.gitmodules" --get-regexp '^submodule\..*\.path$' 2>/dev/null \
        | awk '{print $2}' \
        | while IFS= read -r _p; do
            printf '%s|%s\n' "${_p##*/}" "$_p"
        done
} > "$REPO_LIST"

# ---------------------------------------------------------------------------
# 列举单个仓库
# ---------------------------------------------------------------------------
list_one() {
    _name="$1"
    _rel="$2"
    case "$_rel" in
        .) _repo="$ROOT"; _type="主仓库" ;;
        *) _repo="$ROOT/$_rel"; _type="子模块" ;;
    esac

    _cur=$(git -C "$_repo" rev-parse HEAD 2>/dev/null)
    _short="N/A"
    _branch="-"
    _note=""
    if [ -z "$_cur" ]; then
        _cur=""
        _note="未初始化/无法读取"
    else
        _short=$(git -C "$_repo" rev-parse --short HEAD)
        _branch=$(git -C "$_repo" rev-parse --abbrev-ref HEAD)
        if [ "$_branch" = "HEAD" ]; then
            _branch="(detached)"
        fi
    fi

    # 子模块: 检查主仓库记录的指针与当前检出是否一致
    if [ "$_rel" != "." ] && [ -n "$_cur" ]; then
        _recorded=$(git -C "$ROOT" ls-tree HEAD -- "$_rel" 2>/dev/null | awk '{print $3}')
        if [ -n "$_recorded" ] && [ "$_recorded" != "$_cur" ]; then
            _rec_short=$(git -C "$ROOT" rev-parse --short "$_recorded" 2>/dev/null)
            if [ -n "$_note" ]; then
                _note="$_note; "
            fi
            _note="$_note与主仓库记录($_rec_short)不一致"
        fi
    fi

    printf '%-6s %-18s %-11s %-12s %s\n' "$_type" "$_name" "$_short" "$_branch" "$_note"
    printf '  "%s": "%s",\n' "$_name" "$_cur" >> "$JSON_TMP"
}

info "=================================================================="
info " data-workbench 仓库基线信息一览"
info " 根目录: $ROOT"
info "=================================================================="
printf '%-6s %-18s %-11s %-12s %s\n' "类型" "名称" "哈希" "分支" "备注"
printf '%s\n' "------------------------------------------------------------------"

while IFS='|' read -r _name _rel; do
    [ -n "$_name" ] || continue
    list_one "$_name" "$_rel"
done < "$REPO_LIST"

# ---------------------------------------------------------------------------
# 生成 JSON（完整 40 位哈希，每行一条）
# ---------------------------------------------------------------------------
{
    printf '{\n'
    if [ -s "$JSON_TMP" ]; then
        sed -e '$s/,$//' "$JSON_TMP"
    fi
    printf '}\n'
} > "$OUT_FILE"

info "------------------------------------------------------------------"
info "已生成基线 JSON: $OUT_FILE"
info "把该文件拷贝到外网，覆盖外网仓库的 scripts/bundle_baseline.json，"
info "然后在外网运行 scripts/export_bundles.sh 即可导出增量 bundle。"
info "------------------------------------------------------------------"
cat "$OUT_FILE"
