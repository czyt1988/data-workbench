#!/usr/bin/env bash
# ============================================================================
# worktree-add.sh - 并行开发工作树（.worktree/）创建 / 删除助手
#
# 背景:
#   本项目经常有多个 Agent 会话在同一台机器上并行开发。多个任务挤在同一个工作区
#   里，在途改动会互相污染（切分支、暂存区、未提交改动）。为此每个任务都在
#   <仓库根>/.worktree/<分支名> 下开一个独立工作树，完成后合并回集成分支 dev。
#
#   裸 `git worktree add` 在本项目里不够用，本脚本补上三件事:
#
#   1. 子模块离线初始化（关键）
#      新工作树里裸跑 `git submodule update --init` 不会复用主工作树已有的检出:
#      git 会按 .gitmodules 里的 URL（本项目全部在 gitee）重新 clone，并把 gitdir
#      放到 .git/worktrees/<id>/modules/ 下。内网环境下必然失败。
#      本脚本改用
#        git -c submodule.<名>.url=<本地已有检出> -c protocol.file.allow=always
#            submodule update --init <路径>
#      从任意一个已经检出该子模块的工作树（通常是主工作树）本地克隆: 走本地路径，
#      对象是硬链接，秒级完成、不联网、不改动任何 git config。子模块层层嵌套
#      （SARibbon → qwindowkit → qmsetup → syscmdline），脚本逐层递归。
#
#   2. 复用主工作树预编译的第三方库安装目录（bin_*）
#      CMake 把安装前缀固定为 <仓库根>/bin_<Config>_qt<版本>_<编译器>_<架构>，
#      find_package 找第三方库、构建后复制第三方 DLL 都依赖它。新工作树里该目录
#      不存在，配置期会直接失败（或迫使重编一整套 3rdparty）。脚本把主工作树的
#      bin_* 以目录联接（Windows junction / Linux 软链接）挂进工作树，复用同一份
#      已编译好的第三方库，不重编、不重新下载。
#      ⚠️ 所以工作树里的 bin_* 是**共享目录**: 不要在工作树里执行 cmake --install
#      （会把本工作树的 DA 产物写进共享目录，踩到其它并行会话）。
#
#   3. 联接的安全拆除
#      Windows 下对目录联接做递归删除会删掉目标目录（即主工作树的 bin_*）里的
#      内容。--remove 先拆联接、再删工作树，绝不对联接递归删除。
#      同理: 不要手工 `rm -rf .worktree/<名字>`，用 --remove。
#
#   另外: 首次配置沿用主工作树 build/CMakeCache.txt 的生成器 / Qt 路径 /
#   DA_ENABLE_TESTING 等选项预配置新工作树的 build/ 目录，保证两边构建配置一致
#   （测试目标只有 DA_ENABLE_TESTING=ON 才会编译）。
#
#   提示: 新工作树里固定会有「噪声」文件，不要提交、也不要 checkout 掉:
#     * 检出即已修改的——仓库里有文件提交成了 CRLF，而 .gitattributes 要求 eol=lf
#       （如 src/i18n/backup-da_zh_CN.ts），每次新检出都与索引不一致，checkout 也修不好；
#     * src/DAConfigs.h——配置期由 src/CMakeLists.txt 无条件 configure_file 重新生成到
#       源码树（写入编译日期）。
#   创建时脚本会把第一类实测列出来，两者都不计入「非预期改动」。
#
# 用法:
#   bash scripts/worktree-add.sh <分支> [基线分支]   创建工作树（基线缺省 dev）
#   bash scripts/worktree-add.sh --remove <名字>     删除工作树（保留分支）
#   bash scripts/worktree-add.sh --list              列出工作树
#   bash scripts/worktree-add.sh --help              显示帮助
#
#   创建工作树:
#     <分支>  分支名，约定 feat/<域>-<主题> 或 fix/<主题>。分支已存在则直接检出，
#             否则从 [基线分支] 切出。工作树落在 <主工作树>/.worktree/<分支名把
#             / 换成 ->，目录名与位置固定，与从哪个工作树调用无关。
#     --no-bin-link   不联接主工作树的 bin_*（默认联接: 复用预编译第三方库）
#     --no-configure  不在新工作树预配置 build/（默认预配置，首次构建前可省一次往返）
#     前提: 基线分支记录的子模块提交必须存在于本地克隆源（通常是主工作树）。
#           基线前进到更新过子模块的提交时，先在主工作树把子模块更新到该提交
#           （内网用 scripts/export_bundles.sh 导出的 bundle 更新），否则本地拿不到新提交。
#
#   --remove <名字>:
#     <名字> 是 .worktree/ 下的目录名（如 feat-chart-xyz），也可给完整/相对路径。
#     只删除已注册的工作树，永不删主工作树。分支保留，合并后用 git branch -d 删除。
# ============================================================================

set -euo pipefail

die() { echo "错误: $*" >&2; exit 1; }
warn() { echo "警告: $*" >&2; }
note() { echo "$*"; }

usage() { sed -n '2,/^set -euo/p' "$0" | sed 's/^#//; s/^ //; $d'; }

# 从脚本所在仓库（本仓库的任意工作树）出发，工作树根目录一律落在主工作树下
cd "$(dirname "$0")/.." || die "无法进入仓库根目录"
git rev-parse --git-dir >/dev/null 2>&1 || die "当前目录不在 git 仓库中"

# 平台判断与路径转换（Windows 下创建 junction 需要 Windows 形式的路径）
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) DA_IS_WINDOWS=1 ;;
    *) DA_IS_WINDOWS=0 ;;
esac
to_win_path() {
    if [ "$DA_IS_WINDOWS" = "1" ] && command -v cygpath >/dev/null 2>&1; then
        cygpath -w "$1"
    else
        printf '%s\n' "$1"
    fi
}
# 路径比较用的规范化形式（去掉结尾斜杠、大小写归一）
canon_path() { printf '%s' "$1" | sed 's#/*$##' | tr '[:upper:]' '[:lower:]'; }

# 所有工作树路径，主工作树在最前（porcelain 输出顺序固定）
worktree_paths() { git worktree list --porcelain | sed -n 's/^worktree //p'; }

MAIN_WT=$(worktree_paths | head -n1)
[ -n "$MAIN_WT" ] || die "无法定位主工作树"
cd "$MAIN_WT"
WT_ROOT="$MAIN_WT/.worktree"

is_registered_worktree() {
    local want p
    want=$(canon_path "$1")
    while IFS= read -r p; do
        [ "$(canon_path "$p")" = "$want" ] && return 0
    done < <(worktree_paths)
    return 1
}

# ---------------------------------------------------------------------------
# 子模块: 逐层递归，从本地已有检出克隆（不联网）
#   $1 工作树根目录；$2 当前层级相对工作树根的路径（顶层为空）
# 每层读该层的 .gitmodules，子模块提交记录在父仓库的树里，故读取位置随层级下移。
# ---------------------------------------------------------------------------
init_submodules() {
    local wt=$1 rel=${2:-}
    local dir="$wt${rel:+/$rel}"
    local modfile="$dir/.gitmodules"
    [ -f "$modfile" ] || return 0

    local key path name child sha donor p
    while read -r key path; do
        name=${key#submodule.}
        name=${name%.path}
        child="${rel:+$rel/}$path"
        sha=$(git -C "$dir" ls-tree HEAD "$path" 2>/dev/null | awk '$1 == "160000" {print $3}')
        if [ -z "$sha" ]; then
            continue    # 该提交的树里没有这个子模块（上游已移除）
        fi
        if [ "$(git -C "$wt/$child" rev-parse --verify --quiet HEAD 2>/dev/null)" = "$sha" ]; then
            note "  子模块: $child @ ${sha:0:7}（已就位）"
        else
            donor=""
            while IFS= read -r p; do
                if [ -e "$p/$child/.git" ]; then
                    donor="$p/$child"
                    break
                fi
            done < <(worktree_paths)
            if [ -z "$donor" ]; then
                die "找不到子模块 '$child' 的本地克隆源。请先在有网环境于主工作树执行一次 git submodule update --init（内网可先用 scripts/export_bundles.sh 导出的 bundle 把子模块更新到位）"
            fi
            # protocol.file.allow: git >= 2.38.1 默认拒绝 file 协议的子模块克隆
            # （CVE-2022-39253 缓解），这里克隆源是本地已有检出，属合法用途。
            git -C "$dir" -c "submodule.$name.url=$donor" -c protocol.file.allow=always \
                submodule update --init "$path" >&2 \
                || die "子模块 '$child' 离线初始化失败（本地克隆源 $donor 缺少提交 $sha）。请先在克隆源所在工作树把该子模块更新到 $sha（内网用 bundle）后重试"
            note "  子模块: $child @ ${sha:0:7}（本地克隆自 $donor）"
        fi
        init_submodules "$wt" "$child"
    done < <(git config -f "$modfile" --get-regexp '^submodule\..*\.path$')
}

# ---------------------------------------------------------------------------
# 安装目录: 把主工作树的 bin_* 联接到工作树（复用预编译第三方库）
# ---------------------------------------------------------------------------
link_install_dirs() {
    local wt=$1 src name found=0
    for src in "$MAIN_WT"/bin_*; do
        [ -d "$src" ] || continue
        found=1
        name=$(basename "$src")
        if [ -e "$wt/$name" ] || [ -L "$wt/$name" ]; then
            note "  安装目录: $name（已存在，跳过）"
            continue
        fi
        if [ "$DA_IS_WINDOWS" = "1" ]; then
            powershell -NoProfile -ExecutionPolicy Bypass -Command \
                "New-Item -ItemType Junction -Path '$(to_win_path "$wt/$name")' -Target '$(to_win_path "$src")' | Out-Null" >&2 \
                || die "创建目录联接失败: $wt/$name -> $src"
        else
            ln -s "$src" "$wt/$name" || die "创建软链接失败: $wt/$name -> $src"
        fi
        note "  安装目录: $name -> $src（复用预编译第三方库，不重编/不重新下载）"
    done
    if [ "$found" = "0" ]; then
        warn "主工作树里没有任何 bin_* 安装目录。请先按 build.md 编译安装第三方库，否则新工作树的 CMake 配置会找不到 SARibbon/qwt/ADS 等包"
    fi
}

# 只拆联接本身，绝不递归删除（递归删联接会清空目标目录）
unlink_install_dirs() {
    local wt=$1 p name
    for p in "$wt"/bin_*; do
        [ -e "$p" ] || [ -L "$p" ] || continue
        name=$(basename "$p")
        if [ -L "$p" ]; then
            rm -f "$p" || die "拆除联接失败: $p"
            note "  已拆除联接: $name"
        else
            warn "$name 不是联接（真实目录），保留不删；确认无用后请手动删除"
        fi
    done
}

# ---------------------------------------------------------------------------
# 首次配置: 沿用主工作树 build/CMakeCache.txt 的选项预配置新工作树的 build/
# ---------------------------------------------------------------------------
configure_worktree_build() {
    local wt=$1
    local main_cache="$MAIN_WT/build/CMakeCache.txt"
    if [ ! -f "$main_cache" ]; then
        warn "主工作树没有 build/CMakeCache.txt，跳过预配置。首次构建时请自行确认构建配置（测试需 -DDA_ENABLE_TESTING=ON）"
        return 0
    fi

    local gen prefix platform testing plugins insttype
    gen=$(sed -n 's/^CMAKE_GENERATOR:INTERNAL=//p' "$main_cache" | head -n1)
    platform=$(sed -n 's/^CMAKE_GENERATOR_PLATFORM:INTERNAL=//p' "$main_cache" | head -n1)
    prefix=$(sed -n 's/^CMAKE_PREFIX_PATH:[^=]*=//p' "$main_cache" | head -n1)
    testing=$(sed -n 's/^DA_ENABLE_TESTING:BOOL=//p' "$main_cache" | head -n1)
    plugins=$(sed -n 's/^DA_BUILD_PLUGINS:BOOL=//p' "$main_cache" | head -n1)
    insttype=$(sed -n 's/^DA_INSTALL_CONFIG_TYPE:STRING=//p' "$main_cache" | head -n1)

    local args=(-S "$wt" -B "$wt/build")
    [ -n "$gen" ] && args+=(-G "$gen")
    [ -n "$platform" ] && args+=(-A "$platform")
    [ -n "$prefix" ] && args+=("-DCMAKE_PREFIX_PATH=$prefix")
    [ -n "$testing" ] && args+=("-DDA_ENABLE_TESTING=$testing")
    [ -n "$plugins" ] && args+=("-DDA_BUILD_PLUGINS=$plugins")
    [ -n "$insttype" ] && args+=("-DDA_INSTALL_CONFIG_TYPE=$insttype")

    note "  预配置: cmake ${args[*]}"
    if ! cmake "${args[@]}" >&2; then
        warn "预配置失败，可稍后自行执行 build 脚本（见 build.md）；工作树本身可用"
        return 0
    fi
    note "  预配置完成（与主工作树同一生成器/Qt/测试开关）"
}

# ---------------------------------------------------------------------------
cmd_add() {
    local branch="" base="dev" link_bin=1 do_configure=1
    while [ $# -gt 0 ]; do
        case "$1" in
            --no-bin-link) link_bin=0 ;;
            --no-configure) do_configure=0 ;;
            -*) die "未知选项: $1（见 --help）" ;;
            *) if [ -z "$branch" ]; then branch=$1; else base=$1; fi ;;
        esac
        shift
    done
    [ -n "$branch" ] || die "缺少 <分支>（见 --help）"

    local dirname=${branch//\//-}
    local wt="$WT_ROOT/$dirname"
    if [ -e "$wt" ]; then
        die "工作树目录已存在: $wt（删除用: bash scripts/worktree-add.sh --remove $dirname）"
    fi

    mkdir -p "$WT_ROOT"

    note "=== 创建工作树 ==="
    if git -C "$MAIN_WT" rev-parse --verify --quiet "refs/heads/$branch" >/dev/null; then
        note "  分支 $branch 已存在，直接检出"
        git worktree add "$wt" "$branch"
    else
        git -C "$MAIN_WT" rev-parse --verify --quiet "refs/heads/$base" >/dev/null \
            || die "基线分支不存在: $base"
        note "  从 $base 切出新分支 $branch"
        git worktree add -b "$branch" "$wt" "$base"
    fi

    # 仓库里有文件提交成 CRLF、而 .gitattributes 要求 eol=lf（如 src/i18n/backup-da_zh_CN.ts）:
    # 这类文件在新检出里必然与索引不一致，git checkout 也修不好。用 `git ls-files --eol` 的
    # 静态信息识别（i/crlf + attr 里带 eol=lf），比「创建后 git status 报了什么」可靠——
    # stat 缓存会让同一批文件时隐时现。
    local eol_noise=""
    eol_noise=$(git -C "$wt" ls-files --eol | awk -F'\t' '$1 ~ /i\/crlf/ && $1 ~ /eol=lf/ {print $2}' || true)

    note "=== 初始化子模块（本地离线，不联网） ==="
    init_submodules "$wt"

    if [ "$link_bin" = "1" ]; then
        note "=== 联接主工作树的安装目录（复用预编译第三方库） ==="
        link_install_dirs "$wt"
    fi

    if [ "$do_configure" = "1" ]; then
        note "=== 预配置构建目录 ==="
        configure_worktree_build "$wt"
    fi

    note ""
    note "=== 完成 ==="
    note "  工作树: $wt"
    note "  分支:   $branch（基线 $base）"
    note "  构建:   cd $wt && powershell -NoProfile -File scripts/build.ps1 -Target <模块>"
    note "  测试:   ctest --test-dir $wt/build -C Release -R <测试名> --output-on-failure"
    note "  合并:   在 dev 所在工作树 git merge $branch，然后 bash scripts/worktree-add.sh --remove $dirname"
    note ""
    note "  提醒: 1) bin_* 是指向主工作树的联接（共享预编译第三方库），不要在工作树里执行 cmake --install"
    note "        2) 不要手工 rm -rf 工作树目录（会跟着联接清空主工作树的 bin_*），用 --remove"
    note "        3) 下面的「噪声」文件不要提交、也不要 checkout（checkout 也修不好）:"
    note "           src/DAConfigs.h（配置期由 DAConfigs.h.in 重新生成到源码树，含编译日期）"
    if [ -n "$eol_noise" ]; then
        printf '%s\n' "$eol_noise" | sed 's|^|           |; s|$|（仓库里提交成了 CRLF，换行符噪声）|'
    fi

    # 非预期改动 = git status 报告的文件里剔除上面两类固定噪声
    local noise_file dirty
    noise_file=$(mktemp)
    printf '%s\n' 'src/DAConfigs.h' > "$noise_file"
    if [ -n "$eol_noise" ]; then
        printf '%s\n' "$eol_noise" >> "$noise_file"
    fi
    dirty=$(git -C "$wt" status --porcelain | sed 's/^...//' | grep -vxF -f "$noise_file" || true)
    rm -f "$noise_file"
    if [ -n "$dirty" ]; then
        note ""
        note "  注意: 新工作树里存在非预期改动，请确认:"
        printf '%s\n' "$dirty" | sed 's/^/    /'
    fi
}

cmd_remove() {
    local name=${1:-}
    [ -n "$name" ] || die "缺少 <名字>（见 --help）"

    local wt
    case "$name" in
        /* | [A-Za-z]:/* | [A-Za-z]:\\*) wt=$name ;;   # 绝对路径
        */*) wt=$name ;;
        *) wt="$WT_ROOT/$name" ;;
    esac

    [ -e "$wt" ] || die "工作树目录不存在: $wt"
    if [ "$(canon_path "$wt")" = "$(canon_path "$MAIN_WT")" ]; then
        die "拒绝删除主工作树: $wt"
    fi
    if ! is_registered_worktree "$wt"; then
        # 上一次删除时目录被占用（Windows 常见），git 已摘除注册、只留下空壳目录；
        # 这里直接补删，否则再跑一次 --remove 只会得到「不是已注册的工作树」而无从下手。
        if [ -z "$(ls -A "$wt" 2>/dev/null)" ]; then
            rm -rf "$wt" || die "删除空壳目录失败: $wt（可能有进程占用它的当前目录）"
            note "  已清理上次遗留的空壳目录: $wt"
            return 0
        fi
        die "$wt 不是已注册的工作树（只删除已注册的工作树，避免误删普通目录）"
    fi

    note "=== 删除工作树 ==="
    unlink_install_dirs "$wt"
    if ! git worktree remove --force "$wt"; then
        # Windows 上构建产物可能被 MSBuild/编译器进程短暂占用，目录删不掉；此时工作树
        # 往往已从注册表摘掉、只剩空壳目录。联接已在上面拆除，确认整棵树里没有残留
        # 联接后才允许递归删除（递归删联接会清空它的目标目录）。
        if is_registered_worktree "$wt"; then
            die "工作树仍处于注册状态，未能删除。请确认没有进程（编辑器/终端/MSBuild）停留在 $wt 后重试"
        fi
        local leftover
        leftover=$(find "$wt" -type l 2>/dev/null | head -n1)
        if [ -n "$leftover" ]; then
            die "目录 $wt 里仍有联接/软链接（$leftover），拒绝递归删除；请手工确认后处理"
        fi
        warn "git worktree remove 未能删掉目录（多为被占用），改为删除残留目录"
        local attempt
        for attempt in 1 2 3; do
            rm -rf "$wt" 2>/dev/null && break
            warn "  第 $attempt 次删除失败（目录被占用），1 秒后重试"
            sleep 1
        done
        if [ -e "$wt" ]; then
            warn "  残留目录仍未删除: $wt"
            warn "  工作树已从 git 注册表摘除，剩下的是空壳目录，不影响分支与其它工作树；"
            warn "  常见原因是终端/编辑器的当前目录还停在里面——退出该进程后执行: rm -rf '$wt'"
            note ""
            note "  已删除（git 侧已注销）: $wt"
            note "  分支保留；合并到 dev 后可用 git branch -d <分支> 删除"
            return 0
        fi
    fi
    git worktree prune
    note "  已删除: $wt"
    note "  分支保留；合并到 dev 后可用 git branch -d <分支> 删除"
}

cmd_list() { git worktree list; }

# ---------------------------------------------------------------------------
case "${1:-}" in
    "" | --help | -h | help) usage ;;
    --list) shift; cmd_list "$@" ;;
    --remove) shift; cmd_remove "$@" ;;
    *) cmd_add "$@" ;;
esac
