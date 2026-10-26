# 小狼毫定制版 · 隐藏候选窗 + Tab 预览

基于 [rime/weasel](https://github.com/rime/weasel) 的个人定制分支，上游基线为 `master`（0.17.4）。

打字时不弹候选框，编码直接显示在输入框里；需要看字时按 <kbd>Tab</kbd>，在当前输入框里逐个预览候选，视线不用离开正在写的地方。出厂配置只带小鹤双拼一个方案、「微信配色」一个皮肤。

## 改了什么

| 行为 | 上游 | 本仓库 |
|---|---|---|
| 候选窗口 | 始终显示 | 可用 `style/show_candidate_window: false` 关闭 |
| 关闭后看候选 | — | 按 <kbd>Tab</kbd> 在内嵌预编辑里逐个预览 |
| 出厂方案 | 12 个 | 只有小鹤双拼 |
| 出厂皮肤 | 30 多个 | 只有「微信配色」 |

涉及 5 个文件（+147 / −7），改动只在服务端与面板层，**没有动 IPC 数据结构**，所以新旧进程混跑不会抛 `archive_exception`：

- `RimeWithWeasel/RimeWithWeasel.cpp` —— 新开关、Tab 接管、预览时替换预编辑文本
- `WeaselUI/WeaselPanel.cpp`、`WeaselPanel.h` —— 没有内容可画时标记隐藏
- `WeaselUI/WeaselUI.cpp` —— `Show()` 尊重隐藏标记，避免左上角留下空窗
- `WeaselIPC/Deserializer.h` —— 反序列化失败写日志，不再弹模态框卡住输入

## 一、开启隐藏候选窗口（用户配置）

配置文件：`%APPDATA%\Rime\weasel.custom.yaml`。托盘图标右键 →「用户文件夹」可以直接打开。

最小配置只有一行：

```yaml
patch:
  "style/show_candidate_window": false
```

推荐配置（隐藏候选窗 + 编码内嵌 + 微信配色）：

```yaml
patch:
  # 本仓库新增的键。false = 不弹候选窗；缺省或 true = 与上游一致
  "style/show_candidate_window": false
  # 编码画在输入框里，而不是另起一条预编辑栏
  "style/inline_preedit": true
  # 内嵌显示什么：composition=编码（推荐）/ preview=首个候选 / preview_all=全部候选
  "style/preedit_type": composition
  # 配色，需要 weasel.yaml 的 preset_color_schemes 里存在同名方案
  "style/color_scheme": wechat_green
```

各项含义：

| 键 | 取值 | 作用 |
|---|---|---|
| `style/show_candidate_window` | `true` / `false` | 是否向客户端下发候选列表。设为 `false` 时候选窗不弹，但方案选单（<kbd>Ctrl+&#96;</kbd> / <kbd>F4</kbd>）照常显示 —— 服务端对 switcher 单独放行 |
| `style/inline_preedit` | `true` / `false` | 预编辑画在输入框内，还是另起一行 |
| `style/preedit_type` | `composition` / `preview` / `preview_all` | 内嵌预编辑显示的内容 |

生效步骤：

1. 保存文件（UTF-8）
2. 托盘图标右键 →「重新部署」，或执行 `"C:\Program Files\Rime\weasel-0.17.4\WeaselDeployer.exe" /deploy`
3. 服务端在启动时读配置。界面没变化就退出输入法重新登录，或结束 `WeaselServer.exe` 后重新启动

想恢复默认（重新显示候选窗）：把该项改成 `true`，或整行删掉，再重新部署。

> `style/show_candidate_window` 是本仓库新增的键，在没打这个补丁的官方版本里写了会被忽略，不会报错。

## 二、Tab 预览候选

组合输入中（已有编码、已有候选）时：

- 第一次按 <kbd>Tab</kbd>：内嵌预编辑从「编码」换成「当前高亮候选」，高亮不动，所以看到的就是第一个候选
- 再按 <kbd>Tab</kbd>：等价于 <kbd>↓</kbd>，跳到下一个候选，预编辑同步更新
- <kbd>空格</kbd> 上屏，<kbd>Esc</kbd> 取消；组合结束后预览状态自动复位，下次打字重新显示编码
- <kbd>Ctrl+Tab</kbd>、<kbd>Shift+Tab</kbd> 不拦截，留给系统和方案自己处理

实现上是在 `ProcessKeyEvent` 入口直接接管，不走 `key_binder` —— 因为自带规则里的 `Tab → Shift+Right`（移动分词）会遮蔽配置里的 `Tab → Down`，这也是纯配置改不动 Tab 的原因。

Tab 预览目前没有配置开关。不想要就回退提交 `17e2dd1`。

## 三、安装包与出厂默认

Releases 里的安装包在官方 `install.nsi` 基础上换了出厂配置：

| 项 | 值 |
|---|---|
| 方案 | `double_pinyin_flypy`（小鹤双拼，只此一个） |
| 皮肤 | `wechat_green`（「微信配色」，只此一个） |
| 候选窗 | 关闭 |
| 预编辑 | 内嵌、显示编码、输出简体 |

配色主色 `0x76B100`（ABGR，即 `#00B176` 微信绿）。

安装包用自签名证书（`CN=Dongda Weasel Custom`）签名。**自签名只对本机或已导入证书的机器有效**，别人下载后仍会提示「未知发布者」，需先导入 `certs/weasel-custom.cer` 到「受信任的根证书颁发机构」。

> 升级安装不会覆盖 `%APPDATA%\Rime`。如果那份用户配置来自旧版（比如「薄荷」全套 7 个方案、30 多个皮肤），部署时会按旧的用户配置重新编译，方案和配色又会变多。处理办法是用安装目录 `data\weasel.yaml`、`data\default.yaml` 覆盖用户目录的同名文件，再重新部署。

## 四、从源码构建

已验证的依赖版本：

| 组件 | 版本 |
|---|---|
| Visual Studio | 2026（v145 工具集，MSVC 14.51） |
| Windows SDK | 10.0.26100 |
| Boost | 1.91，需要 `serialization`、`wserialization`、`chrono`、`thread` 四个静态库 |
| librime | 1.17.0 预编译包 |
| NSIS | 3.09 以上（`install.nsi` 用到 `${AtLeastWin11}`） |

步骤：

1. librime：`.\get-rime.ps1 -use dev`，或手工把预编译包的 `dist\include\*.h` 放进 `include\`，`dist\lib\rime.lib` 放进 `lib64\`（x64）和 `lib\`（x86）
2. Boost：解压源码后编出上述四个静态库，产物放 `<boost>\stage\lib`。库名后缀 x64 是 `-x64`，**32 位是 `-x32` 不是 `-x86`**，写错会 `LNK1104`
3. `weasel.props`：以 `weasel.props.template` 为模板，替换 `$BOOST_ROOT`、`$PLATFORM_TOOLSET`、版本号等占位符。本机没装 MFC 时还要给 `rc.exe` 准备一个 `afxres.h` 替身
4. 编译：`MSBuild weasel.sln /p:Configuration=Release /p:Platform=x64`，再编一次 `Platform=Win32`
5. 打包：`makensis output\install.nsi`

## 五、同步上游

```bash
git fetch upstream
git checkout master
git merge upstream/master
```

冲突通常出现在 `RimeWithWeasel.cpp` 的 `_Respond()` 与 `ProcessKeyEvent()` —— 上游改这两个函数比较频繁。合并完把同样的改动 cherry-pick 到 `feat/hide-candidate-window`：

```bash
git checkout feat/hide-candidate-window
git merge master
```

`feat/hide-candidate-window` 与 `master` 内容一致，单独留一个分支是为了向上游提 PR 时干净。

---

## 上游说明（rime/weasel 原文）


基於 中州韻輸入法引擎／Rime Input Method Engine 等開源技術

式恕堂 版權所無

[![Download](https://img.shields.io/github/v/release/rime/weasel)](https://github.com/rime/weasel/releases/latest)
[![Build status](https://github.com/rime/weasel/actions/workflows/commit-ci.yml/badge.svg)](https://github.com/rime/weasel/actions/workflows/commit-ci.yml)
[![GitHub Tag](https://img.shields.io/github/tag/rime/weasel.svg)](https://github.com/rime/weasel)

授權條款：GPLv3

項目主頁：https://rime.im

您可能還需要 RIME 用於其他操作系統的發行版：

  * ibus-rime、fcitx5-rime 或 fcitx-rime 用於 Linux
  * 【鼠鬚管】用於 macOS （64位）

安裝輸入法
----------

本品適用於 Windows 8.1 ~ Windows 11

初次安裝時，安裝程序將顯示「安裝選項」對話框。

若要將【小狼毫】註冊到繁體中文（臺灣）鍵盤佈局，請在「輸入語言」欄選擇「中文（臺灣）」，再點擊「安裝」按鈕。

安裝完成後，仍可由開始菜單打開「安裝選項」更改輸入語言。

使用輸入法
----------

選取輸入法指示器菜單裏的【中】字樣圖標，開始用小狼毫寫字。

可通過快捷鍵 <kbd>Ctrl+`</kbd> 或 <kbd>F4</kbd> 呼出方案選單、切換輸入方式。

定製輸入法
----------

通過 開始菜單 » 小狼毫輸入法 訪問設定工具及常用位置。

用戶詞庫、配置文件位於 `%AppData%\Rime`，可通過菜單中的「用戶文件夾」打開。高水平玩家調教 Rime 輸入法常會用到。

修改詞庫、配置文件後，須「重新部署」方可生效。

定製 Rime 的方法，請參考 Wiki [《定製指南》](https://github.com/rime/home/wiki/CustomizationGuide)。如需定製 Weasel 獨有的樣式和行為，請參考本倉庫 [Wiki 頁面](https://github.com/rime/weasel/wiki)。

致謝
----

### 輸入方案設計：

  * 【朙月拼音】系列及【八股文】詞典
    - 部分數據來源於 CC-CEDICT、Android 拼音、新酷音、opencc 等開源項目
    - 維護者：佛振、瑾昀
  * 【注音／地球拼音】
    - 維護者：佛振、瑾昀
  * 【倉頡五代】
    - 發明人：朱邦復先生
    - 碼表源自 www.chinesecj.com
    - 構詞碼表作者：惜緣

  【五笔】【粵拼】【上海／蘇州吳語】【中古漢語拼音】【國際音標】等衆多方案
  不再以安裝包預裝形式提供。可由 <https://github.com/rime/plum> 下載安裝。

### 程序設計：

  * [佛振](https://github.com/lotem)
  * [鄒旭](https://github.com/zouxu09)
  * [Xiangyan Sun](https://github.com/wishstudio)
  * [Prcuvu](https://github.com/Prcuvu)
  * [nameoverflow](https://github.com/nameoverflow)
  * [fxliang](https://github.com/fxliang)
  * [Azuk 443](https://github.com/determ1ne)

  查看更多 [代碼貢獻者](https://github.com/rime/weasel/graphs/contributors)

### 美術：

  * 圖標設計／[Patricivs](https://github.com/Patricivs)
  * 配色方案／Aben、P1461、Patricivs、skoj、佛振、五磅兔

### 本品引用了以下開源軟件：

  * [Boost C++ Libraries](http://www.boost.org/) (Boost Software License)
  * [curl](https://curl.haxx.se/) (MIT/X derivate license)
  * [google-glog](https://github.com/google/glog) (BSD 3-Clause License)
  * [Google Test](https://github.com/google/googletest) (BSD 3-Clause License)
  * [LevelDB](https://github.com/google/leveldb) (BSD 3-Clause License)
  * [librime](https://github.com/rime/librime) (BSD 3-Clause License)
  * [marisa-trie](https://github.com/s-yata/marisa-trie) (BSD 2-Clause License, LGPL 2.1)
  * [OpenCC / 開放中文轉換](https://github.com/BYVoid/OpenCC) (Apache License 2.0)
  * [plum](https://github.com/rime/plum) (GNU Lesser General Public License v3.0)
  * [WinSparkle](https://github.com/vslavik/winsparkle) (MIT License)
  * [yaml-cpp](https://github.com/jbeder/yaml-cpp) (MIT License)
  * [7-Zip](https://www.7-zip.org) (GNU LGPLv2.1+ with unRAR restriction)

問題與反饋
----------

發現程序有 bug，請到 GitHub 反饋
<https://github.com/rime/weasel/issues>

歡迎提交 pull request
<https://github.com/rime/weasel/pulls>

Rime 輸入法（不限於 Windows 平臺）功能、使用方法與配置相關的問題，請反饋到
<https://github.com/rime/home/issues>

聯繫方式
--------

技術交流，歡迎光臨 [Rime 代碼之家](https://github.com/rime/home)，或致信 Rime 開發者 <rimeime@gmail.com>

謝謝！
