#!/bin/sh
# ============================================================================
# export_bundles.sh - 导出 git bundle 增量同步包
#
# 功能:
#   读取基线文件 scripts/bundle_baseline.json（落后位置的各仓库的基线哈希），
#   对主仓库(data-workbench)与 src/3rdparty 下所有子模块逐一比较当前 HEAD:
#     - 与基线相同        -> 跳过，不导出（例如 qwt 无变更时不生成 qwt.bundle）
#     - 与基线不同        -> 导出增量 bundle（基线..HEAD）
#     - 无基线/基线无法解析 -> 全量导出
#   导出位置: <输出根目录>/<日期YYYYMMDD>/<仓库名称>.bundle
#   同时在该日期目录中生成:
#     - baseline.json : 本次导出后各仓库的目标哈希（落后位置应用全部 bundle 后，
#                       可直接用它替换外网的 scripts/bundle_baseline.json）
#     - README.txt    : 落后位置应用 bundle 的详细操作步骤
#
# 用法:
#   ./export_bundles.sh [输出根目录]
#       输出根目录缺省为 <项目根>/bundle_export，相对路径按当前目录解析；
#       也可用环境变量 BASELINE_JSON 指定其它基线文件。
#
# 基线文件格式（扁平的 "仓库名": "哈希"，每行一条；
# 也可用 scripts/list_repo_hashes.sh 生成）:
#   {
#     "data-workbench": "9ff4918",
#     "qwt": "bb5f462"
#   }
#   哈希值写短哈希或完整 40 位哈希均可；
#   留空 / "-" / "none" 表示该仓库全量导出。
#
# 工作机制:
#   - 主仓库按当前分支名导出（落后位置用 git fetch <bundle> <分支> 应用）；
#     子模块多为 detached HEAD，bundle 记录 HEAD 引用（落后位置用
#     git fetch <bundle> HEAD 导入对象，再由 git submodule update 检出）。
#   - 基线存在但与当前 HEAD 分叉时仍可导出增量（子模块按指针检出不受影响；
#     主仓库在落后位置应用时需 merge 而非 fast-forward）。
# ============================================================================

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd) || exit 1
ROOT=$(cd "$SCRIPT_DIR/.." && pwd) || exit 1
MAIN_NAME="data-workbench"
BASELINE_JSON="${BASELINE_JSON:-$SCRIPT_DIR/bundle_baseline.json}"

info() { printf '%s\n' "$*"; }
warn() { printf '[警告] %s\n' "$*" >&2; }
err()  { printf '[错误] %s\n' "$*" >&2; }

# ---------------------------------------------------------------------------
# 前置检查
# ---------------------------------------------------------------------------
if [ -z "$(command -v git)" ]; then
    err "未找到 git 命令"
    exit 1
fi
if [ ! -f "$BASELINE_JSON" ]; then
    err "基线文件不存在: $BASELINE_JSON"
    err "请参考 scripts/bundle_baseline.json 的格式创建，或运行 scripts/list_repo_hashes.sh 生成"
    exit 1
fi
if [ ! -f "$ROOT/.gitmodules" ]; then
    warn "未找到 .gitmodules，将只导出主仓库"
fi

# 输出根目录: 先创建再转绝对路径
# （git -C <仓库> bundle create <路径> 的相对路径按仓库目录解析，必须用绝对路径）
OUT_ROOT="${1:-$ROOT/bundle_export}"
if ! mkdir -p "$OUT_ROOT"; then
    err "无法创建输出根目录: $OUT_ROOT"
    exit 1
fi
OUT_ROOT=$(cd "$OUT_ROOT" && pwd) || exit 1

DATE=$(date +%Y%m%d)
OUT_DIR="$OUT_ROOT/$DATE"
if ! mkdir -p "$OUT_DIR"; then
    err "无法创建输出目录: $OUT_DIR"
    exit 1
fi

# ---------------------------------------------------------------------------
# 基线读取: 从 JSON 中取仓库名对应的哈希值
# 用 tr 把 { } , 都切成分行，因此单行/多行 JSON 均可解析；
# 值中不能包含花括号与逗号（哈希值天然满足）。
# ---------------------------------------------------------------------------
get_baseline() {
    tr '{},' '\n\n\n' < "$BASELINE_JSON" \
        | sed -n "s/^[[:space:]]*\"$1\"[[:space:]]*:[[:space:]]*\"\([^\"]*\)\".*$/\1/p" \
        | head -n 1
}

key_exists() {
    tr '{},' '\n\n\n' < "$BASELINE_JSON" | grep -F "\"$1\"" >/dev/null 2>&1
}

# 跳过导出时，若当日目录中已存在同名 bundle（早前一次导出留下的），
# 给出提醒，避免目录内容与最新基线状态不一致造成误解
note_stale_bundle() {
    if [ -f "$OUT_DIR/$1.bundle" ]; then
        warn "$1: 本次跳过导出，但目录中已存在 $OUT_DIR/$1.bundle（当日早前导出所留），请结合最新 baseline.json 确认是否为期望内容"
    fi
}

# ---------------------------------------------------------------------------
# 仓库清单: 每行 "名称|相对路径"，主仓库在前
# ---------------------------------------------------------------------------
REPO_LIST=$(mktemp) || exit 1
MANIFEST_TMP=$(mktemp) || exit 1
SUBMODULE_TMP=$(mktemp) || exit 1
trap 'rm -f "$REPO_LIST" "$MANIFEST_TMP" "$SUBMODULE_TMP"' EXIT INT TERM

{
    printf '%s|%s\n' "$MAIN_NAME" "."
    git config --file "$ROOT/.gitmodules" --get-regexp '^submodule\..*\.path$' 2>/dev/null \
        | awk '{print $2}' \
        | while IFS= read -r _p; do
            printf '%s|%s\n' "${_p##*/}" "$_p"
        done
} > "$REPO_LIST"

EXPORT_COUNT=0
SKIP_COUNT=0
ERROR_COUNT=0
EXPORT_DETAIL=""
MAIN_REF=""
MAIN_STATUS="error"    # exported / skip / error

# ---------------------------------------------------------------------------
# 处理单个仓库: 比较基线并按需导出 bundle
# ---------------------------------------------------------------------------
process_repo() {
    _name="$1"
    _rel="$2"
    case "$_rel" in
        .) _repo="$ROOT" ;;
        *) _repo="$ROOT/$_rel" ;;
    esac

    _cur=$(git -C "$_repo" rev-parse HEAD 2>/dev/null)
    if [ -z "$_cur" ]; then
        err "$_name: 无法读取 HEAD（$_repo 不是有效仓库或子模块未初始化），跳过"
        printf '  "%s": "%s",\n' "$_name" "$(get_baseline "$_name")" >> "$MANIFEST_TMP"
        ERROR_COUNT=$((ERROR_COUNT + 1))
        return 0
    fi
    _cur_short=$(git -C "$_repo" rev-parse --short HEAD)

    # 子模块: 检查主仓库记录的指针与当前检出是否一致
    if [ "$_rel" != "." ]; then
        _recorded=$(git -C "$ROOT" ls-tree HEAD -- "$_rel" 2>/dev/null | awk '{print $3}')
        if [ -n "$_recorded" ] && [ "$_recorded" != "$_cur" ]; then
            _rec_short=$(git -C "$ROOT" rev-parse --short "$_recorded" 2>/dev/null)
            warn "$_name: 当前 HEAD ($_cur_short) 与主仓库记录的指针 ($_rec_short) 不一致，请先在主仓库提交子模块指针更新"
        fi
    fi

    # 解析基线
    _raw=$(get_baseline "$_name")
    _full=0
    _base=""
    if [ -z "$_raw" ]; then
        key_exists "$_name" || warn "$_name: 基线文件缺少该条目，将全量导出"
        _full=1
    elif [ "$_raw" = "-" ] || [ "$_raw" = "none" ] || [ "$_raw" = "full" ]; then
        _full=1
    else
        _base=$(git -C "$_repo" rev-parse --verify "${_raw}^{commit}" 2>/dev/null)
        if [ -z "$_base" ]; then
            warn "$_name: 基线 '$_raw' 在该仓库中不存在，退化为全量导出"
            _full=1
        fi
    fi

    if [ "$_full" -eq 0 ]; then
        # 与基线完全一致 -> 不导出
        if [ "$_base" = "$_cur" ]; then
            info "[跳过] $_name: $_cur_short 与基线一致，无变更"
            printf '  "%s": "%s",\n' "$_name" "$_cur" >> "$MANIFEST_TMP"
            SKIP_COUNT=$((SKIP_COUNT + 1))
            [ "$_name" = "$MAIN_NAME" ] && MAIN_STATUS="skip"
            note_stale_bundle "$_name"
            return 0
        fi
        if ! git -C "$_repo" merge-base --is-ancestor "$_base" "$_cur" 2>/dev/null; then
            # 当前 HEAD 落后于基线 -> 无新提交可导出
            if git -C "$_repo" merge-base --is-ancestor "$_cur" "$_base" 2>/dev/null; then
                warn "[跳过] $_name: 当前 HEAD ($_cur_short) 不新于基线 ($_raw)，无提交可导出"
                printf '  "%s": "%s",\n' "$_name" "$_base" >> "$MANIFEST_TMP"
                SKIP_COUNT=$((SKIP_COUNT + 1))
                [ "$_name" = "$MAIN_NAME" ] && MAIN_STATUS="skip"
                note_stale_bundle "$_name"
                return 0
            fi
            warn "$_name: 基线与当前 HEAD 已分叉，继续导出增量（主仓库在落后位置应用时需 merge，子模块按指针检出不受影响）"
        fi
    fi

    # 导出引用: 优先分支名，detached HEAD 时为 HEAD
    _ref=$(git -C "$_repo" rev-parse --abbrev-ref HEAD)
    if [ "$_name" = "$MAIN_NAME" ]; then
        MAIN_REF="$_ref"
    fi

    _bundle="$OUT_DIR/$_name.bundle"
    if [ "$_full" -eq 1 ]; then
        _range="$_ref"
        _desc="全量"
    else
        _range="$_base..$_ref"
        _desc="$(git -C "$_repo" rev-parse --short "$_base")..$_cur_short"
    fi

    if ! _out=$(git -C "$_repo" bundle create "$_bundle" "$_range" 2>&1); then
        err "$_name: 创建 bundle 失败:"
        printf '%s\n' "$_out" >&2
        rm -f "$_bundle"
        printf '  "%s": "%s",\n' "$_name" "$(get_baseline "$_name")" >> "$MANIFEST_TMP"
        ERROR_COUNT=$((ERROR_COUNT + 1))
        return 0
    fi
    if ! git -C "$_repo" bundle verify "$_bundle" >/dev/null 2>&1; then
        err "$_name: bundle 校验失败: $_bundle"
        rm -f "$_bundle"
        printf '  "%s": "%s",\n' "$_name" "$(get_baseline "$_name")" >> "$MANIFEST_TMP"
        ERROR_COUNT=$((ERROR_COUNT + 1))
        return 0
    fi

    _size=$(du -h "$_bundle" 2>/dev/null | awk '{print $1}')
    if [ -n "$_size" ]; then
        _size=" ($_size)"
    fi
    info "[导出] $_name: $_desc$_size"

    printf '  "%s": "%s",\n' "$_name" "$_cur" >> "$MANIFEST_TMP"
    if [ "$_name" = "$MAIN_NAME" ]; then
        MAIN_STATUS="exported"
    else
        printf '   (cd %s && git fetch "$BUNDLE_DIR/%s.bundle" HEAD)\n' "$_rel" "$_name" >> "$SUBMODULE_TMP"
    fi
    EXPORT_COUNT=$((EXPORT_COUNT + 1))
    EXPORT_DETAIL="$EXPORT_DETAIL
  $_name : $_desc"
    return 0
}

# ---------------------------------------------------------------------------
# 逐仓库处理（从文件重定向读取，避免子 shell，计数器才能累积）
# ---------------------------------------------------------------------------
while IFS='|' read -r _name _rel; do
    [ -n "$_name" ] || continue
    process_repo "$_name" "$_rel"
done < "$REPO_LIST"

# ---------------------------------------------------------------------------
# 无任何导出: 清理空目录并退出
# ---------------------------------------------------------------------------
if [ "$EXPORT_COUNT" -eq 0 ] && [ "$ERROR_COUNT" -eq 0 ]; then
    info ""
    info "所有仓库均与基线一致，无需导出 bundle。"
    rmdir "$OUT_DIR" 2>/dev/null
    exit 0
fi

# ---------------------------------------------------------------------------
# 生成 baseline.json: 本次导出后各仓库的目标哈希（含未变更仓库）
# ---------------------------------------------------------------------------
MANIFEST="$OUT_DIR/baseline.json"
{
    printf '{\n'
    if [ -s "$MANIFEST_TMP" ]; then
        sed -e '$s/,$//' "$MANIFEST_TMP"
    fi
    printf '}\n'
} > "$MANIFEST"

# ---------------------------------------------------------------------------
# 生成 README.txt: 落后位置应用步骤
# ---------------------------------------------------------------------------
README="$OUT_DIR/README.txt"
{
    printf '==================================================================\n'
    printf '  bundle 同步包应用说明\n'
    printf '==================================================================\n'
    printf '导出时间 : %s\n' "$(date '+%Y-%m-%d %H:%M:%S')"
    printf '导出目录 : %s\n' "$OUT_DIR"
    printf '\n【导出清单】\n'
    printf '%s\n' "$EXPORT_DETAIL"
    printf '\n【落后位置应用步骤】\n'
    printf '1. 把本目录整体拷贝到落后位置机器任意位置，下文用 $BUNDLE_DIR 表示\n'
    printf '   该目录的实际路径（例如 /home/user/sync/%s）。\n' "$DATE"
    printf '\n'
    if [ "$MAIN_STATUS" = "exported" ]; then
        printf '2. 更新主仓库（在落后位置 %s 仓库根目录执行，本次导出分支: %s）:\n' "$MAIN_NAME" "$MAIN_REF"
        printf '   git fetch "$BUNDLE_DIR/%s.bundle" %s\n' "$MAIN_NAME" "$MAIN_REF"
        printf '   git merge --ff-only FETCH_HEAD\n'
        printf '   （若落后位置当前不在 %s 分支，先执行: git checkout %s）\n' "$MAIN_REF" "$MAIN_REF"
    elif [ "$MAIN_STATUS" = "skip" ]; then
        printf '2. 主仓库与基线一致，本次无需更新。\n'
    else
        printf '2. 主仓库本次导出失败，请先根据外网导出日志排查，勿应用其它 bundle。\n'
    fi
    printf '\n'
    if [ -s "$SUBMODULE_TMP" ]; then
        printf '3. 导入子模块新对象（在落后位置 %s 仓库根目录执行）:\n' "$MAIN_NAME"
        cat "$SUBMODULE_TMP"
    else
        printf '3. 本次无子模块变更。\n'
    fi
    printf '\n'
    printf '4. 检出子模块新版本（仍在落后位置主仓库根目录执行）:\n'
    printf '   git submodule update --init --recursive\n'
    printf '\n'
    printf '5. 同步完成后:\n'
    printf '   - 本目录中的 baseline.json 记录了各仓库同步后的目标哈希，\n'
    printf '     可直接用它替换外网仓库的 scripts/bundle_baseline.json；\n'
    printf '   - 也可在落后位置运行 scripts/list_repo_hashes.sh 重新生成基线 JSON\n'
    printf '     带回外网使用（两者结果应一致）。\n'
    printf '\n'
    printf '【注意事项】\n'
    printf '%s\n' '- bundle 为增量导出，要求落后位置已存在对应基线提交；若 fetch 报'
    printf '  "Repository lacks these prerequisite commits"，说明落后位置实际状态\n'
    printf '  与导出基线不符，请在落后位置运行 scripts/list_repo_hashes.sh 重新生成\n'
    printf '  基线 JSON 带回外网，替换 scripts/bundle_baseline.json 后重新导出。\n'
    printf '%s\n' '- 主仓库若无法 fast-forward（落后位置存在本地提交），改用:'
    printf '  git merge FETCH_HEAD\n'
    printf '%s\n' '- 同日重复运行导出脚本会覆盖同名 bundle 文件；被跳过的仓库若当日'
    printf '  早前导出过，旧 bundle 会保留在目录中，请结合 baseline.json 确认。'
} > "$README"

# ---------------------------------------------------------------------------
# 汇总
# ---------------------------------------------------------------------------
info ""
info "=================================================================="
info "导出完成: 导出 $EXPORT_COUNT 个, 跳过 $SKIP_COUNT 个, 失败 $ERROR_COUNT 个"
info "输出目录: $OUT_DIR"
info "落后位置应用说明: $OUT_DIR/README.txt"
info "同步后基线:   $OUT_DIR/baseline.json"
info "=================================================================="

if [ "$ERROR_COUNT" -gt 0 ]; then
    exit 1
fi
exit 0
